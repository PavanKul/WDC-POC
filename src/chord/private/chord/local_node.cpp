#include "chord/local_node.h"
#include "crypto/sha1.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <dirent.h>
#include "chord/Logger.h"

using namespace std;
namespace fs = std::filesystem;

namespace Chord
{
	LocalNode::LocalNode()
		: self{}
		, fingers{}
		, predecessor{}
		, socket{}
		, requestIdGenerator{}
		, callbacks{}
		, nextFinger{1U}
	{
		// Initialize node
		init();
	}

	bool LocalNode::init()
	{
		// Initialize socket
		if (socket.init() && socket.bind())
		{
			// Get node public address
			// TODO: depending on are visibility
			// Fallback to socket binding address
			self.addr = socket.getAddress();
			getInterfaceAddr(self.addr);

			{
				// Compute sha-1 of address
				uint32 hash[5]; Crypto::sha1(getIpString(self.addr), hash);

				// We use the first 32-bit
				// Uniform distribution
				id = hash[0];
			}

			// Init predecessor, successor and finger table
			predecessor = self;
			for (uint32 i = 0; i < 32; ++i)
				fingers[i] = self;
			stringstream ss;
			ss << "created node ";
            		ss << *self.getInfoString();
            		Logger::getInstance()->chord_print(LOG_LEVEL_INFO, ss.str(), FILE_LOG);
			return true;
		}

		return false;
	}

	const NodeInfo & LocalNode::findSuccessor(uint32 key) const
	{
		const uint32 offset = key - id;

		for (uint32 i = Math::getP2Index(offset, 32); i > 0; --i)
			if (rangeOpen(fingers[i].id, id, key)) return fingers[i];
		
		// Return successor if all other fingers failed
		return successor;
	}

	Request LocalNode::makeRequest(Request::Type type, const NodeInfo & recipient, RequestCallback::CallbackT && onSuccess, RequestCallback::ErrorT && onError, uint32 ttl)
	{
		Request out{type};
		out.sender = self.addr;
		out.recipient = recipient.addr;
		out.flags = 0;
		out.ttl = ttl;
		out.hopCount = 0;

		// Assign unique id
		out.id = requestIdGenerator.getNext();

		// Insert callback
		if (onSuccess || onError)
		{
			ScopeLock _(&callbacksGuard);
			callbacks.insert(out.id, RequestCallback(::move(onSuccess), onError ? ::move(onError) : [this, recipient]() {

				// Check this peer
				checkPeer(recipient);
			}, 5.f));
		}

		return out;
	}

        void LocalNode::copy(const char * file_path, const char * file_name, const NodeInfo target, const int copy_type)
	{
		char path[MAX_FILE_SIZE];
		char *buffer = NULL;
		int file_size = 0;
		stringstream ss;
		ss << "Copy func called, file_path,file_name, dest" ;
            	ss << file_path << ' ' << file_name << ' ' << *getIpString(target.addr);
            	Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
		strcpy(path, file_path);
		strcat(path, file_name);
		ifstream fin(path, ios::in | ios::binary );
		if (!fin) {
			cout<< "File not found!" <<endl;
			return;
		}
		fin.seekg(0, ios::end);
		file_size = fin.tellg();
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
		// When use write for copy files
		// use 'key' = '0'
		
		write(0, buffer, file_size, target, file_name, copy_type);
		buffer = NULL;
	}

	void LocalNode::getWritePath(Request & res, char* filepath)
        {
                if (true == res.isRead)
                {
                       strcpy(filepath, "/Downloads/");

                }
                else
                {
                       if (true == res.isDest)
                       {
                               strcpy(filepath, "/store/");
                               res.isDest = false;
                               res.isSucc1 = true;
                       }
                       else if(true == res.isSucc1)
                       {
                               strcpy(filepath, "/store/copy1/");
                               res.isSucc1 = false;
                               res.isSucc2 = true;
                       }
                       else if(true == res.isSucc2)
                       {
                               strcpy(filepath, "/store/copy2/");
                               res.isSucc2 = false;
                       }
                }
	}


        void LocalNode::write(uint32 key, char* buff, size_t size,
			const NodeInfo target, const char *file_name, int write_type)
        {
    
		NodeInfo dest_node;
	    	//find receipient
		if (0 == key) {
			dest_node = target;
		}
		else {
			auto dest  = lookup(key);
			dest_node = dest.get();
			stringstream ss;
			ss << "RESULT: found key 0x%08x @ ";
			ss << *dest.get().getInfoString();
			Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
		}
	    	Request req = makeRequest(
			Request::WRITE,
			dest_node
		);
            	req.sender = self.addr;
            	req.setSrc<NodeInfo>(self);	
	
	    	//copy buff to req.buff 
		req.setBuff(key, buff, size, file_name);
		if (99 <= write_type) {
			req.isGetFile = true;
			switch(write_type) {
				case 100:
					req.isSucc1 = true;
					req.isDest = false;
					req.isSucc2 = false;
					break;
				case 101:
					req.isSucc2 = true;
					req.isDest = false;
					req.isSucc1 = false;
					break;
				default:;
			}
		}
		else if (1 == write_type) {
			req.isSucc1 = true;
			req.isDest = false;
		} else if (2 == write_type) {
			req.isSucc2 = true;
			req.isDest = false;
			req.isSucc1 = false;
		}

		delete[] buff; // This memory allocation is no longer used
			       // hence deleted
            	socket.write<Request>(req, req.recipient);
        }

        void LocalNode::read( uint32 key, const char* file_name)
        {

                auto dest  = lookup(key);
                auto dest_node = dest.get();
		stringstream ss;
		ss << "RESULT: found key ";
		ss << key << ' ' << *dest.get().getInfoString();
		Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
                Request req = makeRequest(
                        Request::READ,
                        dest_node);
                req.sender = self.addr;
                req.setSrc<NodeInfo>(self);
                req.buff_key = key;
                memset(req.file_name, 0, MAX_FILE_NAME_LENGTH);
                strncpy(req.file_name, file_name, MAX_FILE_NAME_LENGTH-1);
                socket.write<Request>(req, req.recipient);
        }

        void LocalNode::deleteFile( uint32 key, const char* file_name )
        {
        	auto dest  = lookup(key);
        	auto dest_node = dest.get();
		stringstream ss;
		ss << "RESULT: found key ";
		ss << key << ' ' << *dest.get().getInfoString();
		Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
        	Request req = makeRequest(
        			Request::DELETE,
        			dest_node);

        	req.sender = self.addr;
        	req.setSrc<NodeInfo>(self);
        	req.buff_key = key;

        	char clienthost[NI_MAXHOST];  //The clienthost will hold the IP address.
        	char clientservice[NI_MAXSERV];
        	int theErrorCode = getnameinfo(&(req.recipient.__addr), 
				sizeof(req.recipient.__addr), clienthost, sizeof(clienthost), clientservice,
        			sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);

        	std::cout << "LocalNode::deleteFile Receipent :: " << clienthost << std::endl;

        	theErrorCode = getnameinfo(&(req.sender.__addr), 
				sizeof(req.sender.__addr), clienthost, sizeof(clienthost), clientservice,
        			sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);

        	std::cout << "LocalNode::deleteFile Sender :: " << clienthost << std::endl;

        	memset(req.file_name, 0, MAX_FILE_NAME_LENGTH);
        	strncpy(req.file_name, file_name, MAX_FILE_NAME_LENGTH-1);
        	socket.write<Request>(req, req.recipient);
        }
        void LocalNode :: getFileList(uint32 key)
        {
        	Request req{Request::GETFILELIST};

        	if (key > 0)
        	{
            	auto dest  = lookup(key);
            	auto dest_node = dest.get();
            	req.recipient = dest_node.addr;
		stringstream ss;
		ss << "RESULT: found key ";
		ss << key << ' ' << *dest.get().getInfoString();
		Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
        	}
        	else // Send to successor
        		req.recipient = successor.addr;


        	req.sender = self.addr;
        	req.setSrc<NodeInfo>(self);
        	req.buff_key = key;

        	char clienthost[NI_MAXHOST];  //The clienthost will hold the IP address.
        	char clientservice[NI_MAXSERV];
        	int theErrorCode = getnameinfo(&(req.recipient.__addr), sizeof(req.recipient.__addr), clienthost, sizeof(clienthost), clientservice,
        			sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);

        	std::cout << "LocalNode::getFileList Receipent :: " << clienthost << std::endl;

        	theErrorCode = getnameinfo(&(req.sender.__addr), sizeof(req.sender.__addr), clienthost, sizeof(clienthost), clientservice,
        			sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);

        	std::cout << "LocalNode::getFileList Sender :: " << clienthost << std::endl;

        	socket.write<Request>(req, req.recipient);
        }
	bool LocalNode::join(const Ipv4 & peer)
	{
		// Join starts with a lookup request
		Request req = makeRequest(
			Request::LOOKUP,
			NodeInfo{(uint32)-1, peer}
		);
		req.setSrc<NodeInfo>(self);
		req.setDst<uint32>(id);

		// Send lookup request
		socket.write<Request>(req, req.recipient);

		// Wait for response
		// TODO: timeout
		Request res;
		do socket.read(res, res.sender); while (res.id != req.id);

		// Set successor
		successor = res.getDst<NodeInfo>();
		stringstream ss;
		ss << "INFO: connected with successor ";
		ss << *successor.getInfoString();
		Logger::getInstance()->chord_print(LOG_LEVEL_INFO, ss.str(), FILE_LOG);
		// New node added, fetch all keys from successor
		// that belong to this node
		//getSuccFiles("/store/");
		updateForNewNodeAddition();
		return true;
	}

	Promise<NodeInfo> LocalNode::lookup(uint32 key)
	{
		Promise<NodeInfo> out;

		if (rangeOpenClosed(key, id, successor.id))
			out.set(successor);
		else
		{
			// Find closest preceding node
			const NodeInfo & next = findSuccessor(key);

			Request req = makeRequest(
				Request::LOOKUP,
				next,

				// * When key is found, set promise
				[out](const Request & req) mutable {

					out.set(req.getDst<NodeInfo>());
				},

				// * If key is not found, we set an invalid
				// * peer, identified by the wildcard address.
				// * We also check that the finger we asked
				// * for the key is not dead
				[this, out, next]() mutable {

					// Key not found, something went wrong
					out.set(NodeInfo{(uint32)-1, Ipv4::any});

					// Check node
					checkPeer(next);
				},
				3.f
			);
			req.setSrc<NodeInfo>(self);
			req.setDst<uint32>(key);

			// Send lookup request
			socket.write<Request>(req, req.recipient);
		}
		
		return out;
	}

	void LocalNode::getFileList(const char * path, vector<string>& file_list, uint32 check_key)
	{
		uint32 key = 0;
		for (const auto & entry : fs::directory_iterator(path)) {
			string file_name = entry.path().filename();
			key = atoi(file_name.c_str());
                        //if (key != 0 && rangeOpenClosed(key, id, check_key)) {
                        if (key != 0 && key <= check_key) {
                                file_list.push_back(file_name);
                        }
		}
	}

	void LocalNode::copyDirectory(const char * src, const char * dest)
	{
		uint32 key = 0;
                for (const auto & entry : fs::directory_iterator(src)) {
                        string file_name = entry.path().filename();
                        key = atoi(file_name.c_str());
			
                        if (key != 0) {
                                fs::copy(entry.path(), dest, fs::copy_options::overwrite_existing);
                        }
                }
	}

	void LocalNode::copyDirectoryRemote(const char * src, const char * dest, NodeInfo target)
	{
		int key = 0;
                for (const auto & entry : fs::directory_iterator(src)) {
                        string file_name = entry.path().filename();
                        key = atoi(file_name.c_str());
			
                        if (key != 0) {
				copyToRemote(entry.path().c_str(), file_name.c_str(), dest, target);
                        }
                }
	}

	void LocalNode::deleteDirectory(const char * src)
	{
		uint32 key;
                for (const auto & entry : fs::directory_iterator(src)) {
                        string file_name = entry.path().filename();
                        key = atoi(file_name.c_str());
			
                        if (key != 0) {
				removeLocalFile(entry.path());
                        }
                }
	}
	
	void LocalNode::copyToRemote(const char * file_path, const char * file_name, 
			const char * dest, NodeInfo target)
	{
		char *buffer = NULL;
		int file_size = 0;
		stringstream ss;
		ss << "Copy destination called, file_path, file_name, dest target ";
		ss << file_path << ' ' << file_name << ' ' << *getIpString(target.addr);
		Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
		ifstream fin(file_path, ios::in | ios::binary );
		if (!fin) {
			cout<< "File not found!" <<endl;
			return;
		}
		fin.seekg(0, ios::end);
		file_size = fin.tellg();
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
	    	
		Request req = makeRequest(
			Request::WRITE,
			target
		);
		req.isCopyRemote = true;
            	req.sender = self.addr;
            	req.setSrc<NodeInfo>(self);	
		req.setBuff(0, buffer, file_size, file_name);
		strcpy(req.destPath, dest);
		delete[] buffer; // This memory allocation is no longer used
			       // hence deleted
            	socket.write<Request>(req, req.recipient);
	}

	void LocalNode::removeLocalFile(string file_name)
	{
		try {
		if (std::filesystem::remove(file_name))
			std::cout << "file " << file_name << " deleted.\n";
		else
			std::cout << "file " << file_name << " not found.\n";
		}
		catch(const std::filesystem::filesystem_error& err) {
			std::cout << "filesystem error: " << err.what() << '\n';
		}
	}

	void LocalNode::leave()
	{
		char file_name[MAX_FILE_SIZE];
		// Inform successor and predecessor
		// we are leaving the network
		Request req{Request::LEAVE};
		req.sender = self.addr;
		req.setSrc<NodeInfo>(self);

		// Send to successor
		{
			req.recipient = successor.addr;
			socket.write<Request>(req, req.recipient);
		}

		// Send to predecessor
		{
			req.recipient = predecessor.addr;
			socket.write<Request>(req, req.recipient);
		}
	}

	void LocalNode::stabilize()
	{
		// * This op is slightly different from
		// * the one described in the original
		// * paper, for we first notify our current
		// * successor than maybe update it

		Request req = makeRequest(
			Request::NOTIFY,
			successor,
			[this](const Request & req) {
				
				const NodeInfo & target = req.getDst<NodeInfo>();
				if (successor.id == id || rangeOpen(target.id, id, successor.id))
				{
					// Update successor
					setSuccessor(target);
					stringstream ss;
					ss << "new successor is ";
					ss << *successor.getInfoString();
                        		Logger::getInstance()->chord_print(LOG_LEVEL_INFO, ss.str(), FILE_LOG);
				}
			}
		);
		req.setSrc<NodeInfo>(self);

		// Send notify
		socket.write<Request>(req, req.recipient);
	}

	void LocalNode::fixFingers()
	{
		const uint32 key = id + (1U << nextFinger);
		const uint32 i = nextFinger;

		/**
		 * Here there is a piece of code
		 * copied from @ref lookup
		 */
		if (rangeOpenClosed(key, id, successor.id))
		{
			// Update finger
			setFinger(successor, i);
			stringstream ss;
			ss << "updating finger with string ";
			ss << i << ' ' << *fingers[i].getInfoString();
                        Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
		}
		else
		{
			// Update next finger
			const NodeInfo & next = findSuccessor(key);

			Request req = makeRequest(
				Request::LOOKUP,
				next,
				[this, i](const Request & req) {

					// Update finger
					setFinger(req.getDst<NodeInfo>(), i);
					stringstream ss;
					ss << "updating finger with string ";
					ss << i << ' ' << *fingers[i].getInfoString();
                        		Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
				}
				// * checkPeer(next) on error
			);
			req.setSrc<NodeInfo>(self);
			req.setDst<uint32>(key);

			// Send lookup request
			socket.write<Request>(req, req.recipient);
		}

		// Next finger
		nextFinger = ++nextFinger == 32U ? 1U : nextFinger;
	}

	void LocalNode::leaveSucc()
	{
		Request req = makeRequest(
                        Request::LEAVESUCC,
                        successor
                );
		//req.recipient = successor.addr;
		//strcpy(req.file_name, path);
		//req.buff_key = id;
		req.sender = self.addr;
		req.setSrc<NodeInfo>(self);
		socket.write<Request>(req, req.recipient);
		cout<<"In leaveSucc\n";

	}

	void LocalNode::removePeer(const NodeInfo & peer)
	{
		if (peer.id == predecessor.id) {
			// Set predecessor to NIL
			copyDirectory("/store/copy1/", "/store/");
			deleteDirectory("/store/copy1/");
			copyDirectory("/store/copy2/", "/store/copy1/");
			copyDirectoryRemote("/store/copy2/", "/store/copy2/temp/", successor);
			deleteDirectory("/store/copy2/");
			//printf("Predecessor %s removed\n", *peer.getInfoString());
			setPredecessor(self);
			leaveSucc();
		}
		
		if (peer.id == successor.id)
		{
			// ! I don't think this actually works
			stringstream ss;
			ss << "Successor removed ";
			ss << *peer.getInfoString();
			Logger::getInstance()->chord_print(LOG_LEVEL_INFO, ss.str(), FILE_LOG);
			isPredOfRemovedNode = true;
			setSuccessor(self);
			// Reset successor temporarily
			// Do lookup on predecessor
			Request req = makeRequest(
				Request::LOOKUP,
				predecessor,
				[this](const Request & req) {
					
					setSuccessor(req.getDst<NodeInfo>());
					//copyDirectoryRemote("/store/copy1/", "/store/copy2/", successor);
					stringstream ss;
					ss << "new successor is ";
					ss << *successor.getInfoString();
			 		Logger::getInstance()->chord_print(LOG_LEVEL_INFO, ss.str(), FILE_LOG);
				}
			);
			req.setDst<uint32>(id + 1);

			socket.write<Request>(req, req.recipient);
		}

		for (uint32 i = 1; i < 32; ++i)
			if (peer.id == fingers[i].id)
				// Unset finger
				setFinger(self, i);
		stringstream ss;
		ss << "removed node string from local view ";
		ss << *peer.getInfoString();
		Logger::getInstance()->chord_print(LOG_LEVEL_INFO, ss.str(), FILE_LOG);
	}

	void LocalNode::checkPeer(const NodeInfo & peer)
	{
		Request req = makeRequest(
			Request::CHECK,
			peer,
			nullptr,
			[this, peer]() {

				removePeer(peer);
			}
		);
		req.setSrc<NodeInfo>(self);

		// Send request
		socket.write<Request>(req, req.recipient);
		if(true == isPredOfRemovedNode)
		{
                        copyDirectoryRemote("/store/copy1/", "/store/copy2/", successor);
			isPredOfRemovedNode = false;
		}
	}

	void LocalNode::checkPredecessor()
	{
		// Check predecessor
		checkPeer(predecessor);
	}

	void LocalNode::checkRequests(float32 dt)
	{
		// Lock all callbacks
		ScopeLock _(&callbacksGuard);

		for (auto it = callbacks.begin(); it != callbacks.end(); ++it)
		{
			auto id			= it->first;
			auto & callback	= it->second;
			if (callback.tick(dt))
			{
				// Execute error callback
				if (callback.onError) callback.onError();

				// Remove expired callback
				callbacks.remove(it);
				stringstream ss;
				ss << "no reply recieved for request with id: ";
				ss << id;
                	        Logger::getInstance()->chord_print(LOG_LEVEL_INFO, ss.str(), FILE_LOG);
			}
		}
		stringstream ss;
		ss << "pending requests : ";
		ss << callbacks.getCount();
                Logger::getInstance()->chord_print(LOG_LEVEL_INFO, ss.str(), FILE_LOG);	
	}

	void LocalNode::copyToRemoteNew(const char * file_path, const char * file_name,
                        const char * dest, NodeInfo target)
        {
                char *buffer = NULL;
                int file_size = 0;
		stringstream ss;
		ss << "Copy copyToRemoteNew called: ";
		ss << file_path << ' ' << file_name << ' ' << dest << ' ' << *getIpString(target.addr);
		Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
                ifstream fin(file_path, ios::in | ios::binary );
                if (!fin) {
                        cout<< "File not found!" <<endl;
                        return;
                }
                fin.seekg(0, ios::end);
                file_size = fin.tellg();
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

                Request req = makeRequest(
                        Request::UPDATE_NEWNODE_NODE_ADD,
                        target
                );
                req.isCopyRemote = true;
                req.sender = self.addr;
                req.setSrc<NodeInfo>(self);
                req.setBuff(0, buffer, file_size, file_name);
                strcpy(req.destPath, dest);
                delete[] buffer; // This memory allocation is no longer used
                               // hence deleted
                socket.write<Request>(req, req.recipient);
        }

        void LocalNode::updateForNewNodeAddition()
        {
                Request req = makeRequest(
                        Request::UPDATE_SUCCONE_NODE_ADD,
                        successor
                );
                req.buff_key = id;
                req.sender = self.addr;
                req.setSrc<NodeInfo>(self);
                socket.write<Request>(req, req.recipient);
        }

        void LocalNode::getFileListNew(const char * path, vector<string>& file_list, uint32 check_key)
        {
                uint32 key = 0;
                for (const auto & entry : fs::directory_iterator(path)) {
                        string file_name = entry.path().filename();
                        key = atoi(file_name.c_str());
                        if (key != 0 && key <= check_key) {
                                file_list.push_back(file_name);
				//printf("LOG:getFileListNew file pushed:%d, node_key:%d\n", key, check_key);
                        }
                }
        }

        bool LocalNode::copyFilesToRemote(const char * src, const char * dest,
			            NodeInfo target, vector<string>& file_list)
        {
                int key = 0;
		bool isAnyFileSent = false;
		auto itr_begin = file_list.begin();
		auto itr_end = file_list.end();
	        string src_str(src);			
                for (; itr_begin != itr_end; ++itr_begin) {
                        string file_name = *itr_begin;
                        key = atoi(file_name.c_str());

                        if (key != 0) {
                                string src_file = src_str + file_name;
                                copyToRemoteNew(src_file.c_str(), file_name.c_str(), dest, target);
				isAnyFileSent = true;
                        }
                }
		return isAnyFileSent;
        }

        bool LocalNode::copyFilesToLocal(const char * src, const char * dest,
                                                 vector<string>& file_list)
        {
                int key = 0;
                bool isAnyFileSent = false;
                auto itr_begin = file_list.begin();
                auto itr_end = file_list.end();
		string src_str (src);

                for (; itr_begin != itr_end; ++itr_begin) {
                        string file_name = *itr_begin;
                        key = atoi(file_name.c_str());

                        if (key != 0) {
				string new_src = src_str + file_name;
				fs::copy(new_src.c_str(), dest, fs::copy_options::overwrite_existing);
                        }
                }
        }

        bool LocalNode::copyDirectoryRemoteNew(const char * src,
			               const char * dest, NodeInfo target)
        {
                int key = 0;
		bool isAnyFileSent = false;
                for (const auto & entry : fs::directory_iterator(src)) {
                        string file_name = entry.path().filename();
                        key = atoi(file_name.c_str());

                        if (key != 0) {
                                copyToRemoteNew(entry.path().c_str(), file_name.c_str(), dest, target);
				isAnyFileSent = true;
                        }
                }
		return isAnyFileSent;
        }

        void LocalNode::removeLocalFiles(string& path, vector<string>& file_list)
	{
                auto itr_begin = file_list.begin();
		auto itr_end = file_list.end();
		for(; itr_begin != itr_end; ++itr_begin)
		{
			string file_name = path + *itr_begin;
                        removeLocalFile(file_name);
		}
	}

	bool LocalNode::transferFilesToNewNode(const Request & req, 
			              const char * path, bool isKeyChkReq = false)
	{
		bool isAnyFileSent = false;
		vector<string> file_list;
		char file_path[MAX_FILE_NAME_SIZE];
		file_list.clear();
		const NodeInfo &new_node = req.getSrc<NodeInfo>();

		if (true == isKeyChkReq)
		{
		        getFileListNew(path, file_list, req.buff_key);
                        //isAnyFileSent = copyFilesToRemote (path, path, predecessor, file_list);
			isAnyFileSent = copyFilesToRemote (path, path, new_node, file_list);
			if(isAnyFileSent)
			{
				string path_str(path);
				copyFilesToLocal("/store/", "/store/copy1/", file_list);
				removeLocalFiles(path_str, file_list);
			}
                        
		}
		else
		{
                        isAnyFileSent = copyDirectoryRemoteNew(path, path, new_node);
			if(isAnyFileSent)
			{
				std::string copy1("/store/copy1/");
				std::string path_str (path);
				if (path_str == copy1)
				{
			                copyDirectory("/store/copy1/", "/store/copy2/");	
				}
				deleteDirectory(path);
			}
		}

		return isAnyFileSent;
		//TODO:take care of removing files
	}


	void LocalNode::handleRequest(const Request & req)
	{
		switch (req.type)
		{
	#if BUILD_DEBUG
		case Request::PING:
			//printf("LOG: received PING from %s with id 0x%08x\n", *getIpString(req.sender), req.id);
			break;
	#endif

		case Request::REPLY:
			//printf("LOG: received REPLY from %s with id 0x%08x\n", *getIpString(req.sender), req.id);
			handleReply(req);
			break;

		case Request::LOOKUP:
			//printf("LOG: received LOOKUP from %s with id 0x%08x and hop count = %u\n", *getIpString(req.sender), req.id, req.hopCount);
			handleLookup(req);
			break;

		case Request::NOTIFY:
			//printf("LOG: received NOTIFY from %s with id 0x%08x\n", *getIpString(req.sender), req.id);
			handleNotify(req);
			break;

		case Request::LEAVE:
			//printf("LOG: received LEAVE from %s with id 0x%08x\n", *getIpString(req.sender), req.id);
			handleLeave(req);
			break;
		
		case Request::CHECK:
			//printf("LOG: received CHECK from %s with id 0x%08x\n", *getIpString(req.sender), req.id);
			handleCheck(req);
			break;
		
		case Request::WRITE:{
			stringstream ss;
			ss << "received WRITE from string with id ";
			ss << *getIpString(req.sender) << ' ' << req.id;
			Logger::getInstance()->chord_print(LOG_LEVEL_INFO, ss.str(), FILE_LOG);
            		handleWrite(req);
            		break;}

                case Request::READ:{
			stringstream ss;
                        ss << "received READ from string with id ";
                        ss << *getIpString(req.sender) << ' ' << req.id;
                        Logger::getInstance()->chord_print(LOG_LEVEL_INFO, ss.str(), FILE_LOG);
                        handleRead(req);
                        break;}

		case Request::DELETE:{
			stringstream ss;
                        ss << "received DELETE from string with id ";
                        ss << *getIpString(req.sender) << ' ' << req.id;
                        Logger::getInstance()->chord_print(LOG_LEVEL_INFO, ss.str(), FILE_LOG);
			handleDelete(req);
			break;}

                case Request::GETFILELIST:{
			stringstream ss;
                        ss << "received GETFILELIST from with id path ";
                        ss << *getIpString(req.sender) << ' ' << req.id;
                        Logger::getInstance()->chord_print(LOG_LEVEL_INFO, ss.str(), FILE_LOG);
			handleGetFileList(req);
			break;}

		case Request::LEAVESUCC:{
			stringstream ss;
                        ss << "received LEAVESUCC from string with id ";
                        ss << *getIpString(req.sender) << ' ' << req.id;
                        Logger::getInstance()->chord_print(LOG_LEVEL_INFO, ss.str(), FILE_LOG);
			handleLeaveSucc(req);
			break;}

		case Request::UPDATE_SUCCONE_NODE_ADD:{
			stringstream ss;
                        ss << "recieved UPDATE_SUCCONE_NODE_ADD from string with id: ";
                        ss << *getIpString(req.sender) << ' ' << req.id;
                        Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
			handleSuccOneForNodeAdd(req);
			break;}

                case Request::UPDATE_SUCCTWO_NODE_ADD:{
			stringstream ss;
                        ss << "recieved UPDATE_SUCCTWO_NODE_ADD from string with id: ";
                        ss << *getIpString(req.sender) << ' ' << req.id;
                        Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
			handleSuccTwoForNodeAdd(req);
                        break;}

                case Request::UPDATE_SUCCTHREE_NODE_ADD:{
			stringstream ss;
                        ss << "recieved UPDATE_SUCCTHREE_NODE_ADD from string with id: ";
                        ss << *getIpString(req.sender) << ' ' << req.id;
                        Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
			handleSuccThreeForNodeAdd(req);
                        break;}

                case Request::UPDATE_NEWNODE_NODE_ADD:{
			stringstream ss;
                        ss << "recieved UPDATE_NEWNODE_NODE_ADD from string with id ";
                        ss << *getIpString(req.sender) << ' ' << req.id;
                        Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
			handleNewNodeForNodeAdd(req);
                        break;}

		default:
			stringstream ss;
			ss << "received UNKOWN from string with id ";
			ss << *getIpString(req.sender) << ' ' << req.id;
			Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
			break;
		}
	}

	void LocalNode::handleLeaveSucc(const Request & req)
	{
		copyDirectory("/store/copy2/", "/store/copy1/");
		copyDirectoryRemote("/store/copy2/", "/store/copy2/", successor);
		deleteDirectory("/store/copy2/");	
		copyDirectory("/store/copy2/temp/", "/store/copy2/");
		deleteDirectory("/store/copy2/temp/");	
	}

	void LocalNode::handleReply(const Request & req)
	{
		// Lock all callbacks
		ScopeLock _(&callbacksGuard);

		// Find associated callback
		auto it = callbacks.find(req.id);
		if (it != callbacks.nil())
		{
			// Execute callback
			auto callback = it->second;
			if (callback.onSuccess) callback.onSuccess(req);

			// Remove
			callbacks.remove(it);
		}
	}

	void LocalNode::handleLookup(const Request & req)
	{
		const NodeInfo & src = req.getSrc<NodeInfo>();
		const uint32 key = req.getDst<uint32>();

		// If successor is succ(key) or successor is null
		if (rangeOpenClosed(key, id, successor.id))
		{
			// Reply to source node
			Request res{req};
			res.type = Request::REPLY;
			res.sender = self.addr;
			res.recipient = src.addr;
			res.setDst<NodeInfo>(successor);
			res.reset();

			socket.write<Request>(res, res.recipient);
		}
		else
		{
			// Find closest preceding node
			const NodeInfo & next = findSuccessor(key);

			// Break infinite loop
			if (next.id == id)
			{
				// Reply to source node
				Request res{req};
				res.type = Request::REPLY;
				res.sender = self.addr;
				res.recipient = src.addr;
				res.setDst<NodeInfo>(self);
				res.reset();

				socket.write<Request>(res, res.recipient);
			}
			else
			{
				// Forward request
				Request fwd{req};
				fwd.sender	= self.addr;
				fwd.recipient = next.addr;
				
				socket.write<Request>(fwd, fwd.recipient);
			}
		}
	}

	void LocalNode::handleNotify(const Request & req)
	{
		const NodeInfo & src = req.getSrc<NodeInfo>();

		// Reply with current predecessor
		Request res{req};
		res.type = Request::REPLY;
		res.sender = self.addr;
		res.recipient = src.addr;
		res.setDst<NodeInfo>(predecessor);

		socket.write<Request>(res, res.recipient);
		
		// if predecessor is nil or n -> (predecessor, self)
		if (predecessor.id == id || rangeOpen(src.id, predecessor.id, id))
		{
			// Update predecessor
			setPredecessor(src);
			stringstream ss;
			ss << "new predecessor ";
			ss << *predecessor.getInfoString();
                        Logger::getInstance()->chord_print(LOG_LEVEL_INFO, ss.str(), FILE_LOG);
		}
	}

	void LocalNode::handleLeave(const Request & req)
	{
		// Remove leaving node from local view
		removePeer(req.getSrc<NodeInfo>());
	}

	void LocalNode::handleCheck(const Request & req)
	{
		// Reply back if we are still alive (I guess we are!)
		const NodeInfo & src = req.getSrc<NodeInfo>();

		Request res{req};
		res.type = Request::REPLY;
		res.sender = self.addr;
		res.recipient = src.addr;

		socket.write<Request>(res, res.recipient);

		// TODO: chain checks along a lookup path
	}


	void LocalNode::handleWrite(const Request & req)
    	{
        	Request sender{req};
		Request res{req};
		char filepath[MAX_FILE_NAME];
		string filename;
		bool isSucc2 = res.isSucc2;
		bool isSucc1 = res.isSucc1;
		bool isDest = res.isDest;
		getWritePath(res, filepath);

		// Handling copy to remote first
		if (req.isCopyRemote) {
			strcpy(filepath, req.destPath);
			strcat(filepath, req.file_name);
			stringstream ss;
			ss << "Copy to remote received: + req.destPath ";
			ss << req.destPath << ' ' << req.file_name;
			Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
			ofstream fout(filepath, ios::out | ios::binary);
    			fout.write (&req.file_buff[0], req.buff_size);
			fout.close();

			return;
		}

		printf("getWritePath: %s\n", filepath);
                if (true == req.isRead)
		{
			if (0 == req.buff_key) {
				strcpy(filepath, "/store/");
				filename = req.file_name;
				strcat(filepath, req.file_name);
			}
			else {
				//strcpy(filepath, "/Downloads/");
				filename = req.file_name;
			}

		}
                else
		{
			if (0 == req.buff_key) {
				filename = string(req.file_name);
				//strcat(filepath, filename.c_str());
				stringstream ss;
				ss << "Recieved file as copy: ";
				ss << filepath << ' ' << req.file_name;
                        	Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
			}
			else
				filename = to_string(req.buff_key);
		}
		//if (0 != req.buff_key)
		strcat(filepath, filename.c_str());
		ofstream fout(filepath, ios::out | ios::binary);
    		fout.write (&req.file_buff[0], req.buff_size);
		fout.close();
		if (req.isGetFile || isSucc2)
			return;
		if((true != req.isRead) && 
				(res.isSucc1 || res.isSucc2))
		{
			if (0 == req.buff_key) {
				int copy_type;
				string pathstr = string(req.file_name);
				string filename = pathstr.substr(pathstr.find_last_of("/\\") + 1);
				if (isDest) {
					strcpy(filepath, "/store/copy1/");
					copy_type = 1;
				}
				else {
					strcpy(filepath, "/store/copy2/");
					copy_type = 2;
				}
				copy(filepath, filename.c_str(), successor, copy_type);
			} else {
                        	res.sender = self.addr;
                        	res.recipient = successor.addr;

                		socket.write<Request>(res, res.recipient);
			}
		}
    	}

        void LocalNode::handleRead(const Request & req)
        {
                Request res{req};
                char file_name[MAX_FILE_NAME];
		strcpy(file_name,"/store/");
                //string name = to_string(req.buff_key);
		string name = to_string(req.buff_key);
		//file_name += name;
                if (0 != req.buff_key) {
			//file_name += to_string(req.buff_key);
			strcat(file_name, name.c_str());
		} else {
			//file_name += string(req.file_name);
			strcat(file_name, req.file_name);
		}
                int file_size = 0;
		stringstream ss;
		ss << "handleRead key,file_name, req.buff_key ";
		ss << req.buff_key << ' ' << file_name;
		Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
                ifstream fin(file_name, ios::in | ios::binary );
		if (!fin)
		{
                        //file_name = "/store/copy1/" + name;
			strcat(file_name, name.c_str());
			fin.open(file_name, ios::in | ios::binary );
		        if (!fin)
                        {
                                //file_name = "/store/copy2/" + name;
				strcat(file_name, name.c_str());
                                fin.open(file_name, ios::in | ios::binary );
		        }

		}
                if (fin)
                {
                        fin.seekg(0, ios::end);
                        file_size = fin.tellg();
                        printf("File Size = %d", file_size);
                        fin.seekg(0, ios::beg);
                        if (MAX_FILE_SIZE >= file_size) {
                                fin.read(&res.file_buff[0], file_size);
                                res.buff_size = file_size;
                        }
                        else
                        {
				stringstream ss;
				ss << "File is larger than the max allowed size ";
				Logger::getInstance()->chord_print(LOG_LEVEL_ERROR, ss.str(), FILE_LOG);
                        }
                        fin.close();
                }
                else
                {
			stringstream ss;
			ss << "File is not present in /store/ ";
			Logger::getInstance()->chord_print(LOG_LEVEL_ERROR, ss.str(), FILE_LOG);
                }
                res.isRead = true;
                res.type = Request::WRITE; 
                socket.write<Request>(res, res.sender);
        }

	void LocalNode::handleDelete(const Request & req)
	{
        	Request res{req};
		char filepath[MAX_FILE_NAME];
		
		getWritePath(res, filepath);
		string file_name = string(filepath);
        	file_name += to_string(res.buff_key);

        	try {
        		if (std::filesystem::remove(file_name))
        			std::cout << "file " << file_name << " deleted.\n";
        		else
        			std::cout << "file " << file_name << " not found.\n";
        	}
        	catch(const std::filesystem::filesystem_error& err) {
        		std::cout << "filesystem error: " << err.what() << '\n';
        	}
		if(res.isSucc1 || res.isSucc2)
		{
			res.sender = self.addr;
			res.recipient = successor.addr;

			socket.write<Request>(res, res.recipient);
		}
        }

        void LocalNode::handleGetFileList(const Request & req)
        {
        	Request res{req};
        	if (req.isFileList)
        	{
        		cout << "Received file list :: " << req.file_buff << endl;
        	}
        	else
        	{
            	string dirName("/store/");
            	DIR *dr;
            	struct dirent *en;
            	dr = opendir(dirName.c_str()); //open all directory
            	memset (res.file_buff, 0, MAX_FILE_NAME);
            	if (dr) {
            		while ((en = readdir(dr)) != NULL)
            		{
            			long num = 0;
            			num = atol(en->d_name);
            			if ( num > 0)
            			{
            				strncat (res.file_buff, en->d_name, strlen(en->d_name));
            				strncat (res.file_buff, ",", strlen(","));
            			}
            		}
            		cout << "File list :: " << res.file_buff << endl; //print all directory name
            		closedir(dr); //close all directory
            	}
                res.isFileList = true;
                res.type = Request::GETFILELIST;
                socket.write<Request>(res, res.sender);
        	}
        }

        void LocalNode::handleSuccOneForNodeAdd(const Request & req)
        {
                bool hasStoreTransfer = false;
		bool hasCopyOneTransfer = false;
		bool hasCopyTwoTransfer = false;
                Request req_succ{req}; 
		//update /copy2
		hasCopyTwoTransfer = transferFilesToNewNode(req, "/store/copy2/");

		//update /copy1
                hasCopyOneTransfer = transferFilesToNewNode(req, "/store/copy1/");

		//update /store
		if (req.buff_key < id)
		{
	                hasStoreTransfer = transferFilesToNewNode(req, "/store/", true);
		}

		if (hasCopyTwoTransfer || hasCopyOneTransfer || hasStoreTransfer)
		{
			req_succ.type = Request::UPDATE_SUCCTWO_NODE_ADD;
		//	req_succ.isCopyTwoUpdateRequired = hasCopyTwoTransfer;
			req_succ.isCopyOneUpdateRequired = hasCopyOneTransfer;
			req_succ.isStoreUpdateRequired = hasStoreTransfer;
                        socket.write<Request>(req_succ,  successor.addr);
		}

	}

        void LocalNode::handleSuccTwoForNodeAdd(const Request & req)
        {
		vector<string> file_list;
		Request req_succ{req};

                if (true == req.isCopyOneUpdateRequired)
		{
                        deleteDirectory("/store/copy2/");
		}

		if (true == req.isStoreUpdateRequired)
		{
			file_list.clear();
                        getFileListNew("/store/copy1/", file_list, req.buff_key);
			copyFilesToLocal("/store/copy1/", "/store/copy2/", file_list);
			req_succ.type = Request::UPDATE_SUCCTHREE_NODE_ADD;
			//handleSuccThreeForNodeAdd(req_succ, successor);
			string path("/store/copy1/");
			removeLocalFiles(path, file_list);
			socket.write<Request>(req_succ,  successor.addr);
		}
        }

        void LocalNode::handleSuccThreeForNodeAdd(const Request & req)
        {
                vector<string> file_list;
		getFileListNew("/store/copy2/", file_list, req.buff_key);
                string path("/store/copy2/");
		removeLocalFiles(path, file_list);
        }

        void LocalNode::handleNewNodeForNodeAdd(const Request & req)
        {
                char filepath[MAX_FILE_NAME];
		memset(filepath, 0, MAX_FILE_NAME);
		stringstream ss;
		ss << "handleNewNodeForNodeAdd with dest, filename and buffer size: ";
		ss << req.destPath << ' ' << req.file_name << ' ' << req.buff_size;
                Logger::getInstance()->chord_print(LOG_LEVEL_DEBUG, ss.str(), FILE_LOG);
                strcpy(filepath, req.destPath);
                strcat(filepath, req.file_name);
		//printf("handleNewNodeForNodeAdd: filepath:%s\n", filepath);


                ofstream fout(filepath, ios::out | ios::binary);
                fout.write (&req.file_buff[0], req.buff_size);
                fout.close();

        }
} // namespace Chord
