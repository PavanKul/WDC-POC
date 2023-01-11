#include "chord/local_node.h"
#include "crypto/sha1.h"
#include <iostream>
#include <fstream>
#include <filesystem>

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

			printf("INFO: created node %s\n", *self.getInfoString());
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
		printf("Copy func called, file_path = %s, file_name = %s\n", file_path, file_name);
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
			printf("RESULT: found key 0x%08x @ [%s]\n", key, *dest.get().getInfoString());
		}
	    	Request req = makeRequest(
			Request::WRITE,
			dest_node
			//NodeInfo{(uint32)-1, dest.addr}
		);
            	req.sender = self.addr;
            	req.setSrc<NodeInfo>(self);	
	
	    	//copy buff to req.buff 
		req.setBuff(key, buff, size, file_name);
		if (99 == write_type)
			req.isGetFile = true;
		else if (1 == write_type) {
			req.isSucc1 = true;
			req.isDest = false;
		} else if (2 == write_type) {
			req.isSucc2 = true;
			req.isDest = false;
		}

		delete[] buff; // This memory allocation is no longer used
			       // hence deleted
            	socket.write<Request>(req, req.recipient);
        }

        void LocalNode::read( uint32 key, const char* file_name)
        {

                auto dest  = lookup(key);
                auto dest_node = dest.get();
                printf("RESULT: found key 0x%08x @ [%s]\n", key, *dest.get().getInfoString());
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
        	printf("RESULT: found key 0x%08x @ [%s]\n", key, *dest.get().getInfoString());
        	Request req = makeRequest(
        			Request::DELETE,
        			dest_node);

        	req.sender = self.addr;
        	req.setSrc<NodeInfo>(self);
        	req.buff_key = key;

        	char clienthost[NI_MAXHOST];  //The clienthost will hold the IP address.
        	char clientservice[NI_MAXSERV];
        	int theErrorCode = getnameinfo(&(req.recipient.__addr), sizeof(req.recipient.__addr), clienthost, sizeof(clienthost), clientservice,
        			sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);

        	std::cout << "LocalNode::deleteFile Receipent :: " << clienthost << std::endl;

        	theErrorCode = getnameinfo(&(req.sender.__addr), sizeof(req.sender.__addr), clienthost, sizeof(clienthost), clientservice,
        			sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);

        	std::cout << "LocalNode::deleteFile Sender :: " << clienthost << std::endl;

        	memset(req.file_name, 0, MAX_FILE_NAME_LENGTH);
        	strncpy(req.file_name, file_name, MAX_FILE_NAME_LENGTH-1);
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

		printf("INFO: connected with successor %s\n", *successor.getInfoString());
		// New node added, fetch all keys from successor
		// that belong to this node
		getSuccFiles("/store/");
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


	void LocalNode::getSuccFiles(const char * path)
	{
		Request req = makeRequest(
                        Request::GETFILELIST,
                        successor
                );
		//req.recipient = successor.addr;
		strcpy(req.file_name, path);
		req.buff_key = id;
		req.sender = self.addr;
		req.setSrc<NodeInfo>(self);
		socket.write<Request>(req, req.recipient);
		cout<<"In getSuccFiles \n";
	}

	void LocalNode::getFileList(const char * path, vector<string>& file_list, uint32 check_key)
	{
		int key = 0;
		for (const auto & entry : fs::directory_iterator(path)) {
			string file_name = entry.path().filename();
			key = atoi(file_name.c_str());
                        if (key != 0 && key <= check_key) {
                                file_list.push_back(file_name);
                        }
		}
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
		vector<string> file_list;
		if (successor.id != id) {
			getFileList("/store/", file_list);
			for (int i = 0; i < file_list.size(); i++) {
				printf("LOG: sending file %s\n", file_list[i].c_str());
				copy("/store/", file_list[i].c_str(), successor, 0);
				strcpy(file_name, "/store/");
				strcat(file_name, file_list[i].c_str());
				removeLocalFile(string(file_name));
			}
                }
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

					//printf("LOG: new successor is %s\n", *successor.getInfoString());
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
			
			//printf("LOG: updating finger #%u with %s\n", i, *fingers[i].getInfoString());
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

					//printf("LOG: updating finger #%u with %s\n", i, *fingers[i].getInfoString());
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

	void LocalNode::removePeer(const NodeInfo & peer)
	{
		if (peer.id == predecessor.id)
			// Set predecessor to NIL
			setPredecessor(self);
		
		if (peer.id == successor.id)
		{
			// ! I don't think this actually works

			// Reset successor temporarily
			setSuccessor(self);

			// Do lookup on predecessor
			Request req = makeRequest(
				Request::LOOKUP,
				predecessor,
				[this](const Request & req) {
					
					setSuccessor(req.getDst<NodeInfo>());

					//printf("LOG: new successor is %s\n", *successor.getInfoString());
				}
			);
			req.setDst<uint32>(id + 1);

			socket.write<Request>(req, req.recipient);
		}

		for (uint32 i = 1; i < 32; ++i)
			if (peer.id == fingers[i].id)
				// Unset finger
				setFinger(self, i);
		
		printf("LOG: removed node %s from local view\n", *peer.getInfoString());
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

				//printf("LOG: no reply received for request with id %08x\n", id);
			}
		}

		//printf("LOG: %llu pending requests\n", callbacks.getCount());
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
		
		case Request::WRITE:
            		printf("LOG: received WRITE from %s with id 0x%08x\n", *getIpString(req.sender), req.id);
            		handleWrite(req);
            		break;

                case Request::READ:
                        printf("LOG: received READ from %s with id 0x%08x\n", *getIpString(req.sender), req.id);
                        handleRead(req);
                        break;

		case Request::DELETE:
			printf("LOG: received DELETE from %s with id 0x%08x\n", *getIpString(req.sender), req.id);
			handleDelete(req);
			break;

		case Request::GETFILELIST:
			printf("LOG: received GETFILELIST from %s with id 0x%08x\n", *getIpString(req.sender), req.id);
			handleGetFiles(req);
			break;

		default:
			printf("LOG: received UNKOWN from %s with id 0x%08x\n", *getIpString(req.sender), req.id);
			break;
		}
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

	void LocalNode::handleGetFiles(const Request & req)
	{
		char file_path[MAX_FILE_NAME_SIZE];
		const NodeInfo & src = req.getSrc<NodeInfo>();
        	Request send{req};
		vector<string> file_list;
		printf("handleGetFiles called\n");
		strcpy(file_path, req.file_name);
		if (req.buff_key < id) {
			getFileList(file_path, file_list, req.buff_key);
			for (int i = 0; i < file_list.size(); i++) {
				printf("LOG: sending file %s\n", file_list[i].c_str());
				copy(req.file_name, file_list[i].c_str(), src, 99);
				strcpy(file_path, req.file_name);
				strcat(file_path, file_list[i].c_str());
				removeLocalFile(string(file_path));
			}
		}
		if (strcmp(req.file_name, "/store/") == 0)
			strcpy(send.file_name, "/store/copy1/");
		else if (strcmp(req.file_name, "/store/copy1") == 0)
			strcpy(send.file_name, "/store/copy2/");
		else
			return;
		send.sender = self.addr;
		send.recipient = successor.addr;

                socket.write<Request>(send, send.recipient);
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

			//printf("LOG: new predecessor is %s\n", *predecessor.getInfoString());
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
				strcat(filepath, filename.c_str());
				printf("Received file %s as copy\n", filepath);
			}
			else
				filename = to_string(req.buff_key);
		}
		if (0 != req.buff_key)
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
                string file_name("/store/");
                //string name = to_string(req.buff_key);
		string name = to_string(req.buff_key);
		file_name += name;
                if (0 != req.buff_key) {
			file_name += to_string(req.buff_key);
		} else {
			file_name += string(req.file_name);
		}
                int file_size = 0;
                ifstream fin(file_name, ios::in | ios::binary );
		if (!fin)
		{
                        file_name = "/store/copy1/" + name;
			fin.open(file_name, ios::in | ios::binary );
		        if (!fin)
                        {
                                file_name = "/store/copy2/" + name;
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
                                printf("File is larger than the max allowed size\n");
                        }
                        fin.close();
                }
                else
                {
                        printf("File is not present in /store/\n");
                }
                res.isRead = true;
                res.type = Request::WRITE; 
                socket.write<Request>(res, res.sender);
        }

	void LocalNode::handleDelete(const Request & req)
	{
        	Request res{req};
        	string file_name("/store/");
        	file_name += to_string(req.buff_key);

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
} // namespace Chord
