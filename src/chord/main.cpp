#include "coremin.h"
#include "math/math.h"
#include "hal/threading.h"
#include "misc/command_line.h"
#include "chord/chord.h"
#include "chord/types.h"
#include <fstream>
#include <iostream>
#include <string>
#include <signal.h>
#include "crypto/sha1.h"
#include "crypto/sha1.h"
#include <mysql/mysql.h>

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

/*void fileStore()
{
	std::vector<char> buffer = file_map.serialize();
	ofstream os ("/store/filemap.dat", ios::binary);
	int size = buffer.size();
	os.write((const char*)&size, 4);
	os.write((const char*)&buffer[0], size * sizeof(char));
	os.close();
	cout << "Map saved" << endl;
}*/

void interrupt_handler(int sig)
{
    cout << "\nReceived interruption : " << sig << endl;
    //fileStore();
	localNode.leave();
	exit(1);

}

  MYSQL *conn = NULL;
  MYSQL_ROW row;
 
  int mySQL_func(char query[], MYSQL_RES **res)
{
      //MYSQL_ROW row;
 
      char *server = "172.30.44.119";
      char *user = "root";
      char *password = "Master@1234";
      char *database = "File";
 
      conn = mysql_init(NULL);
 
          if (!mysql_real_connect(conn, server, user, password,database, 0, NULL, 0)) {
                  fprintf(stderr, "%s\n", mysql_error(conn));
                  exit(1);
          }
 
          if (mysql_query(conn, query)) {
                  fprintf(stderr, "%s\n", mysql_error(conn));
                  exit(1);
          }
          //*res = mysql_use_result(conn);
 
          /*if (conn)
          {
                  mysql_close(conn);
                  conn = NULL;
          }*/
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
	
//	Chord::LocalNode localNode;
  
	std::vector<char> buffer;
  	/*ifstream is("/store/filemap.dat", ios::binary);
	if (is) {
  		int size;
  		is.read((char*)&size, 4);
 	 	buffer.resize(size);
  		is.read((char*)&buffer[0], size * sizeof(char));
 	 	file_map.deserialize(buffer);
	}*/

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
	printf("User options: \np = Print Info, \nl = Lookup, \nq = Leave node, \
			\nw = Write file, \nr = Read file, \ns = Show files, \nd = Delete file\n");

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

  			/*std::vector<char> buffer = file_map.serialize();
  			ofstream os ("/store/filemap.dat", ios::binary);
  			int size = buffer.size();
  			cout << "Size of the vector buffer: " << size << endl;
  			os.write((const char*)&size, 4);
  			os.write((const char*)&buffer[0], size * sizeof(char));
  			os.close();*/
			localNode.leave();
			break;
		}

		case 'w':
		{
                        /*char query[100] = "\0";
			string path = "";
			Chord::NodeInfo temp;
			cout << "Please enter file name (full path) to be stored: " << endl;
			cin >> path;
			string filename = path.substr(path.find_last_of("/\\") + 1);
			auto it = file_map.find(filename);
			if(it != file_map.end())
			{
				cout << it->first << " already exists" << endl;
			}
			else
			{
				char *buffer = NULL;
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
				localNode.write(hash[0], buffer, file_size, temp); // temp will not be used
				string pathstr = string(path);
				string filename = pathstr.substr(pathstr.find_last_of("/\\") + 1);
				cout <<"File added: "<< pathstr << endl;
				file_map.insert(pair<string, uint32>(filename, hash[0]));
				if (buffer)
				{
					buffer = NULL;
				}
			}
			break;*/
			string path = "";
                        cout << "Please enter file name (full path) to be stored: " << endl;
                        cin >> path;

                        char *buffer = NULL;
                        char query[100] = "\0";
			Chord::NodeInfo temp;
                        int file_size = 0, flag=0;
                        uint32 hash[5], key;
                        string pathstr = string(path);
                        string fileName = pathstr.substr(pathstr.find_last_of("/\\") + 1);
                        sprintf(query, "SELECT * FROM FileList;");
                        MYSQL_RES *result = NULL;
                        mySQL_func(query, &result);
                        result = mysql_use_result(conn);

                        while ((row = mysql_fetch_row(result)) != NULL) {

                                if(strcmp(fileName.c_str(),row[1]) == 0) {
                                        key = atoi(row[0]);
                                        cout<<"The key is "<<"    "<<key<<endl;
                                        cout<<"The File is present in database"<<"    "<<endl;
                                        flag=1;
                                        break;
                                }
                        }
                        mysql_free_result(result);
                        mysql_close(conn);

                        if (flag == 0) {

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
                        Crypto::sha1(buffer, hash);
                        printf("File key: 0x%08x\n", hash[0]);
                        //localNode.write(hash[0], buffer, file_size);
			localNode.write(hash[0], buffer, file_size, temp); // temp will not be used
                        //string pathstr = string(path);
                        //string fileName = pathstr.substr(pathstr.find_last_of("/\\") + 1);
                        cout <<"File added: "<< pathstr << endl;
                        cout <<"Filename added: "<< fileName << endl;

                        sprintf(query, "INSERT INTO `FileList` (`key`, `FileName`) VALUES (%d, '%s');", hash[0], fileName.c_str());
                        MYSQL_RES *result = NULL;
                        mySQL_func(query, &result);
                        mysql_close(conn);
                        }
                        if (buffer)
                        {
                                buffer = NULL;
                        }
                        break;
		}

		case 'd':
		{
			/*string fileName;
			cout << "Enter the filename to delete : ";
			cin >> fileName;
			uint32 key = 0;
			auto it = file_map.find(fileName);
			if(it != file_map.end())
			{
				key = it->second;
				cout << "Key-value pair present : " << fileName << "->" << key << std::endl;
				localNode.deleteFile(key, fileName.c_str());
			}
			else
			{
				cout << "File  not present in map" << std::endl;
			}
			break;*/
			string fileName;
                        int flag=0;
                        char query[100] = "\0";
                        cout << "Enter the filename to delete : ";
                        cin >> fileName;
                        uint32 key = 0;
                        sprintf(query, "SELECT * FROM FileList;");
                        MYSQL_RES *result = NULL;
                        mySQL_func(query, &result);
                        result = mysql_use_result(conn);

                        while ((row = mysql_fetch_row(result)) != NULL) {

                                if(strcmp(fileName.c_str(),row[1]) == 0) {
                                        key = atoi(row[0]);
                                        cout<<"The key is "<<"    "<<key<<endl;
                                        localNode.deleteFile(key, fileName.c_str());
                                        flag=1;
                                        break;
                                }
                        }
                        mysql_free_result(result);
                        mysql_close(conn);
                         if (flag == 1) {

                                        sprintf(query, "DELETE FROM `FileList` WHERE `FileName` = '%s';", fileName.c_str());
                                        MYSQL_RES *result = NULL;
                                        mySQL_func(query, &result);
                        		mysql_close(conn);
                         }
                                if (flag == 0)
                                {
                                        cout<<"The file is not present"<<endl;
                                }
                        break;
		}

		case 'r':
		{
			/*string fileName;
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
			break;*/
			string fileName;
                        cout <<"Enter the filename: ";
                        cin>>fileName;
                        uint32 key;
                        char query[100] = "\0";
                        sprintf(query, "SELECT * FROM FileList;");
                        MYSQL_RES *result = NULL;
                        mySQL_func(query, &result);
                        result = mysql_use_result(conn);

                        while ((row = mysql_fetch_row(result)) != NULL) {

                                if(strcmp(fileName.c_str(),row[1]) == 0)
                                {
                                        key = atoi(row[0]);
                                        cout<<"The key is "<<"    "<<key<<endl;
                                        localNode.read(key, fileName.c_str());
                                        break;
                                }
                                else
                                {
                                        cout<<"The file is not present"<<endl;
                                }
                        }
                        memset(query,0,100);
                        mysql_free_result(result);
                        mysql_close(conn);
                        break;
		}

		case 's':
		{
			/*cout << "List of files added using this node:\n";
			for (auto i : file_map)
        			cout << "'" << i.first << "',   " << endl;
			break;*/
			char query[100] = "\0";
                        cout << "List of files added using this node:\n";
                        sprintf(query, "SELECT * FROM FileList;");
                        MYSQL_RES *result = NULL;
                        mySQL_func(query, &result);
                        result = mysql_use_result(conn);

                                while ((row = mysql_fetch_row(result)) != NULL){
                                cout<<"key:"<<row[0]<<" "<<"FileName:"<<row[1]<<std::endl;
                                }
                        mysql_free_result(result);
                        mysql_close(conn);
                        break;
		}

		default:
			break;
		}
	} while(c != 'q');

	return 0;
}

