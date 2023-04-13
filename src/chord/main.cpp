#include "coremin.h"
#include "math/math.h"
#include "hal/threading.h"
#include "misc/command_line.h"
#include "chord/chord.h"
#include "chord/types.h"
#include "DB/DBManager.h"
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>
#include <signal.h>
#include "crypto/sha1.h"
#include "crypto/sha1.h"

#define MAX_FILEPATH_SIZE 4096

/// The global allocator used by default
Malloc * gMalloc = nullptr;

/// Thread manager
ThreadManager * gThreadManager = nullptr;

/// Global argument parser
CommandLine * gCommandLine = nullptr;
  
// Global map for filename and key
// This should be stored in a file
// to make it persistent
//Chord::serializable_map<std::string, int> file_map;
Chord::LocalNode localNode;

using namespace std;

void interrupt_handler(int sig)
{
    cout << "\nReceived interruption : " << sig << endl;
	localNode.leave();
	exit(1);
}

int32 main(int32 argc, char ** argv)
{
	//////////////////////////////////////////////////
	// Application setup
	//////////////////////////////////////////////////

	// Create globals	
	Memory::createGMalloc();
	gThreadManager = new ThreadManager();
	gCommandLine = new CommandLine(argc, argv);
	
	Net::Ipv4 peer;
	if (CommandLine::get().getValue("input", peer, [](const String & str, Net::Ipv4 & peer){

		Net::parseIpString(peer, *str);
	})) {
		printf("Peer: %s \n", *getIpString(peer));
		localNode.join(peer);
	}

	auto receiver = RunnableThread::create(new Chord::ReceiveTask(&localNode), "Receiver");
	auto updater = RunnableThread::create(new Chord::UpdateTask(&localNode), "Updater");

	int rs = 0;

	char line[256] = {};
	printf("User options: \np = Print Info, \nl = Lookup, \nq = Leave node, \
			\nw = Write file, \nr = Read file, \ns = Show files, \nd = Delete file,"
			"\ng = get file list from any other node\n");

	char c; do
	{
		printf("Enter your choice: ");

		//signal(SIGINT, interrupt_handler);
		//signal(SIGSEGV, interrupt_handler);

		switch (c = getc(stdin))
		{
		case 'p':
		{
			localNode.printInfo();
			break;
		}

		case 'l':
		{
			uint32 key;
			scanf("%x", &key);

			auto peer = localNode.lookup(key);
			printf("RESULT: found key 0x%08x @ [%s]\n", key, *peer.get().getInfoString());
			break;
		}
		
		case 'q':
		{
			localNode.leave();
			break;
		}

		case 'w':
		{
			string path = "";
                        cout << "Please enter file name (full path) to be stored: " << endl;
                        cin >> path;

                        char *buffer = NULL;
			char *keyBuff = NULL;
                        char query[100] = "\0";
			Chord::NodeInfo temp;
                        int file_size = 0, flag=0;
                        uint32 hash[5], key;
                        string pathstr = string(path);
                        string fileName = pathstr.substr(pathstr.find_last_of("/\\") + 1);
			string inputKey;
			if (fileName == pathstr)
			{
                                std::filesystem::path cwd = std::filesystem::current_path();
				inputKey = cwd.string();
				inputKey = inputKey + "/" + fileName;
			}
			else
				inputKey = pathstr;
			cout << "inputKey:"<<inputKey<<endl;
			if (localNode.dbIns)
				flag = localNode.dbIns->ifFilePresent((char*)fileName.c_str(), key);
                        if (flag == 0) {

                        ifstream fin(path, ios::in | ios::binary );
                        if (!fin) {
                                cout<< "File not found!" <<endl;
                                break;
                        }
                        fin.seekg(0, ios::end);
                        file_size = fin.tellg();
                        std::cout << "File Size = "<<file_size<<" File name:"<<fileName<<
				            " File path:"<<pathstr<<endl;
                        if(buffer = new char[file_size])
                        {
                                memset(buffer, '0', file_size);
                        }
                        else{
                                //TODO: handle out of memory
                        }
                        fin.seekg(0, ios::beg);
                        fin.read(buffer, file_size);
                        fin.close();
                        if (MAX_FILE_SIZE < file_size) {
                                printf("File is larger than the max allowed size\n");
                                break;
                        }

			int inKeySize = inputKey.size();
                        if(keyBuff = new char[inKeySize])
                        {
                                memset(keyBuff, '0', inKeySize);
				strcpy(keyBuff, inputKey.c_str());
                        }

                        Crypto::sha1(keyBuff, hash);

        				flag = 1;
        				while (flag)
        				{
						if (localNode.dbIns)
							flag = localNode.dbIns->ifHashPresent(hash[0]);
        					if (flag)
        						hash[0] = hash[0] + 10;
        				}

                        printf("File key: 0x%08x\n", hash[0]);

			//localNode.write(hash[0], buffer, file_size, temp); // temp will not be used
			localNode.write_new(hash[0], buffer, file_size, temp);
			if (localNode.dbIns)
				flag = localNode.dbIns->insertToFileList((char*)fileName.c_str(), hash[0]);
                        }
                        if (buffer)
                        {
                                buffer = NULL;
                        }
                        break;
		}

		case 'd':
		{
			string fileName;
			int flag=0;
			char query[100] = "\0";
			cout << "Enter the filename to delete : ";
			cin >> fileName;
			uint32 key = 0;
			if (localNode.dbIns)
				flag = localNode.dbIns->ifFilePresent((char*)fileName.c_str(), key);
			if (flag == 1) {
				localNode.deleteFile(key, fileName.c_str());
				if (localNode.dbIns)
					flag = localNode.dbIns->deleteFromFileList((char*)fileName.c_str());
			}else
			{
				cout<<"The file is not present"<<endl;
			}
			break;
		}

		case 'r':
		{
			string fileName;
			cout <<"Enter the filename: ";
			cin>>fileName;
			uint32 key;
			int flag = 0;
			if (localNode.dbIns)
				flag = localNode.dbIns->ifFilePresent((char*)fileName.c_str(), key);
			if (flag == 1)
				//localNode.read(key, fileName.c_str());
				localNode.read_new(key, fileName.c_str());
			break;
		}

		case 's':
		{
			cout << "List of files present:\n";
			if (localNode.dbIns)
				localNode.dbIns->showFileList();
			break;
		}

		case 'g':
		{
			int key = 0;
			string s = "";
			cout << "Enter the key or 0 if targetting to successor : " << endl;
			cin >> key;
			localNode.getFileList(key);
			break;
		}

		default:
			break;
		}
	} while(c != 'q');

	return 0;
}

