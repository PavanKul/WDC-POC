#pragma once

#include "async/async.h"

#include "chord_fwd.h"
#include "types.h"
#include "request.h"
#include "math/uuid_generator.h"
#include <vector>

#define MAX_FILE_NAME 2048
#define MAX_FILE_NAME_SIZE 296

using namespace std;
namespace Chord
{
	/**
	 * @class LocalNode chord/local_node.h
	 * 
	 * A chord node running on the local machine
	 */
	class LocalNode
	{
		friend ReceiveTask;
		friend UpdateTask;

	protected:
		union
		{
			/// Local node info
			NodeInfo self;

			/// Local node id
			uint32 id;
		};

		union
		{
			/// Finger table
			/// Fingers with the same id of the local
			/// node are not valid
			NodeInfo fingers[32];

			/// Successor, a.k.a. first finger
			NodeInfo successor;
		};

		/// Predecessor node
		NodeInfo predecessor;

		/// Node UDP socket
		SocketDgram socket;

		/// Request id generator
		UUIdGenerator<uint16> requestIdGenerator;

		/// Request map
		Map<uint16, RequestCallback> callbacks;

		/// The index of the finger we'll update
		uint32 nextFinger;

		/// To identify whether the node is predecessor of removed node
		bool isPrecOfRemodedNode = false;

		/// Mutex variables
		/// @{
		CriticalSection predecessorGuard;
		CriticalSection fingersGuard[32];
		CriticalSection callbacksGuard;
		/// @}
	
	public:
		/// Default constructor
		LocalNode();
		
		/// Get node public address
		FORCE_INLINE const Ipv4 & getPublicAddress() const
		{
			return self.addr;
		}

		//////////////////////////////////////////////////
		// Thread-safe setters
		//////////////////////////////////////////////////
		
		/// Set finger
		FORCE_INLINE void setFinger(const NodeInfo & node, uint32 i)
		{
			ScopeLock _(fingersGuard + i);
			fingers[i] = node;
		}

		/// Set successor
		FORCE_INLINE void setSuccessor(const NodeInfo & node)
		{
			ScopeLock _(fingersGuard + 0U);
			setFinger(node, 0);
		}

		FORCE_INLINE void setPredecessor(const NodeInfo & node)
		{
			ScopeLock _(&predecessorGuard);
			predecessor = node;
		}

	protected:
		/// Node initialization
		bool init();
	
	protected:
		/**
		 * Find successor for key in finger table
		 * 
		 * @param [in] key key to lookup
		 * @return successor node
		 */
		const NodeInfo & findSuccessor(uint32 key) const;

		/**
		 * Forge a request spawning from this node
		 * 
		 * @param [in] type request type
		 * @param [in] recipient request target
		 * @return forged request
		 */
		Request makeRequest(
			Request::Type type,
			const NodeInfo & recipient,
			RequestCallback::CallbackT && onSuccess = nullptr,
			RequestCallback::ErrorT && onError = nullptr,
			uint32 ttl = (uint32)-1
		);

	public:
		//////////////////////////////////////////////////
		// Chord API
		//////////////////////////////////////////////////

		/**
		 * Join chord ring (blocking operation)
		 * 
		 * @param [in] peer address of a known peer
		 * @return join status
		 */
		bool join(const Ipv4 & peer);

		/**
		 * Look up key in chord ring
		 * 
		 * @param [in] key key to lookup
		 * @return future successor info
		 */
		Promise<NodeInfo> lookup(uint32 key);

		 /**
                 * write will send a write request to destination 
                 *
                 * @param [in] a  key, buffer & buffer size
                 * @return void
                 */
                 void write(uint32 key, char* buff, size_t size,
				 const NodeInfo target, const char *file_name = NULL, int write_type = 0);

		 /**
                 * getWritePath  will set the file path for write operation
                 *
                 * @param [in] a req structure and filepath for storing path
                 * @return void
                 */
		 void getWritePath(Request & res, char* filepath);

		 /**
                 * copy will send a file to destination
                 *
                 * @param [in] file_name
                 * @return void
                 */
                 void copy(const char *file_path, const char * file_name, const NodeInfo target, const int copy_type);

		 void removeLocalFile(string file_name);
                 void getFileList(const char * path, vector<string>& file_list, uint32 key = 0xFFFFFFFF);
                 void copyDirectory(const char * src, const char * dest);
		 void copyDirectoryRemote(const char * src, const char * dest, NodeInfo target);
		 void copyToRemote(const char * file_path, const char *file_name,
				 const char * dest, NodeInfo target);
		 void deleteDirectory(const char * src);
		 void leaveSucc();

                 /**
                 * read will send a read request to destination
                 *
                 * @param [in] a key (corresponds to a file)
                 * @return void
                 */
                 void read(uint32 key, const char* file_name);

                 /**
                  * delete will send a delete request to destination
                  *
                  * @param [in] a key (corresponds to a file)
                  * @return void
                  */
                void deleteFile(uint32 key, const char* file_name);

		/**
		 * Leave chord ring
		 */
		void leave();

		/**
		 * Get the file list from a node from the cluster.
		 * If no node specified, list will be fetched from
		 * successor node*/
		void getSuccFiles(const char * path);

	protected:
		/**
		 * Stabilize node and notify successor
		 */
		void stabilize();

		/**
		 * Fix current next finger
		 */
		void fixFingers();

		/**
		 * Remove remote node from the local view
		 * 
		 * @param [in] peer node to remove
		 */
		void removePeer(const NodeInfo & peer);

		/**
		 * Check peer node
		 * 
		 * @param [in] peer chord node to check
		 */
		void checkPeer(const NodeInfo & peer);

		/**
		 * Check predecessor to ensure it is still alive
		 */
		void checkPredecessor();

		/**
		 * Check expired requests
		 * 
		 * @param [in] dt delta time from last check
		 */
		void checkRequests(float32 dt);

	protected:
		/**
		 * Process incoming request
		 * 
		 * @param [in] req incoming request
		 * @{
		 */
		void handleRequest(const Request & req);
		void handleReply(const Request & req);
		void handleLookup(const Request & req);
		void handleNotify(const Request & req);
		void handleLeave(const Request & req);
		void handleCheck(const Request & req);
		void handleWrite(const Request & req);
		void handleRead(const Request & req);
		void handleDelete(const Request & req);
		void handleGetFileList(const Request & req);
		void handleGetFiles(const Request & req);
		void handleLeaveSucc(const Request & req);
		/// @}
		
	public:
		/// Prints node info to the std output
		void printInfo() const
		{
			printf("# -----------------\n");
			printf("# node %s\n", *self.getInfoString());
			printf("# ---- | ----------\n");
			printf("# pred | %s\n", predecessor.id == id ? "self" : *predecessor.getInfoString());
			printf("# succ | %s\n", successor.id == id ? "self" : *successor.getInfoString());

			for (uint32 i = 1; i < 32; ++i)
				printf("#   %02u | %s\n", i, fingers[i].id == id ? "self" : *fingers[i].getInfoString());
		}

	protected:
		/**
		 * Returns whether number falls in (circular) range or not
		 * 
		 * @param [in] n test variable
		 * @param [in] a,b range delimiters
		 * @return true if test variables falls in range
		 * @{
		 */
		template<typename T>
		static FORCE_INLINE bool rangeOpen(T n, T a, T b)
		{
			return	(a < b && (n > a && n < b)) ||
					(a > b && (n > a || n < b));
		}
		
		template<typename T>
		static FORCE_INLINE bool rangeClosed(T n, T a, T b)
		{
			return	(a < b && (n >= a && n <= b)) ||
					(a > b && (n >= a || n <= b));
		}
		
		template<typename T>
		static FORCE_INLINE bool rangeOpenClosed(T n, T a, T b)
		{
			return	(a < b && (n > a && n <= b)) ||
					(a > b && (n > a || n <= b));
		}
		
		template<typename T>
		static FORCE_INLINE bool rangeClosedOpen(T n, T a, T b)
		{
			return	(a < b && (n >= a && n < b)) ||
					(a > b && (n >= a || n < b));
		}
		/// @}
	};
} // namespace Chord
