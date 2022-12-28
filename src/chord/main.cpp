#include "coremin.h"
#include "math/math.h"
#include "hal/threading.h"
#include "misc/command_line.h"
#include "chord/chord.h"
#include <fstream>
#include <iostream>
#include <string>
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
Chord::serializable_map<std::string, int> file_map;

using namespace std;
int32 main(int32 argc, char ** argv)
{
	//////////////////////////////////////////////////
	// Application setup
	//////////////////////////////////////////////////

	// Create globals	
	Memory::createGMalloc();
	gThreadManager = new ThreadManager();
	gCommandLine = new CommandLine(argc, argv);
	
	Chord::LocalNode localNode;
  
	std::vector<char> buffer;
  	ifstream is("/store/filemap.dat", ios::binary);
	if (is) {
  		int size;
  		is.read((char*)&size, 4);
 	 	buffer.resize(size);
  		is.read((char*)&buffer[0], size * sizeof(char));
 	 	file_map.deserialize(buffer);
	}

	Net::Ipv4 peer;
	if (CommandLine::get().getValue("input", peer, [](const String & str, Net::Ipv4 & peer){

		Net::parseIpString(peer, *str);
	})) {
		printf("Peer: %s \n", *getIpString(peer));
		localNode.join(peer);
	}

	auto receiver = RunnableThread::create(new Chord::ReceiveTask(&localNode), "Receiver");
	auto updater = RunnableThread::create(new Chord::UpdateTask(&localNode), "Updater");

	char line[256] = {};
	printf("User options: p = Print Info, l = Lookup, q = Leave node, \n \
			w = Write file, r = Read file, s = Show files\n"); 
	char c; do
	{
		printf("Enter your choice: ");
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

  			std::vector<char> buffer = file_map.serialize();
  			ofstream os ("/store/filemap.dat", ios::binary);
  			int size = buffer.size();
  			cout << "Size of the vector buffer: " << size << endl;
  			os.write((const char*)&size, 4);
  			os.write((const char*)&buffer[0], size * sizeof(char));
  			os.close();
			localNode.leave();
			break;
		}

		case 'w':
		{
			char path[MAX_FILEPATH_SIZE];
			printf("Please enter file name (full path) to be stored: ");
			scanf("%s", path);
			char *buffer;
			int file_size = 0;
			ifstream fin(path, ios::in | ios::binary );
			if (!fin) {
        			cout<< "File not found!" <<endl;
				break;
    			}
			fin.seekg(0, ios::end);
			file_size = fin.tellg();
			std::cout << "File Size = "<<file_size<<endl;
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
			uint32 hash[5]; Crypto::sha1(buffer, hash);
			printf("File key: 0x%08x\n", hash[0]);
			localNode.write(hash[0], buffer, file_size);
			string pathstr = string(path);
			string filename = pathstr.substr(pathstr.find_last_of("/\\") + 1);
			cout <<"File added: "<< pathstr << endl;
			file_map.insert(pair<string, uint32>(filename, hash[0]));
			break;
		}

		case 'r':
		{
			string fileName;
			cout <<"Enter the filename: ";
			cin>>fileName;
			uint32 key;
			auto it = file_map.find(fileName);
			if(it != file_map.end())
			{
				key = it->second;
				cout << "Key-value pair present : " << fileName << "->" << key << std::endl;
				localNode.read(key, fileName.c_str());
			}
			else
			{
                                cout << "File  not present in map" << std::endl;
			}
			break;
		}

		case 's':
		{
			cout << "List of files added using this node:\n";
			for (auto i : file_map)
        			cout << "'" << i.first << "',   " << endl;
			break;
		}

		default:
			break;
		}
	} while(c != 'q');

	return 0;
}

