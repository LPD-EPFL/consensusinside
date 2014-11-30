#ifndef _T_Replica_h
#define _T_Replica_h 1

#include <pthread.h>

//#include "T_Cryptography.h"
#include "types.h"
#include "T_Rep_info.h"
//#include "Digest.h"
#include "T_Node.h"
#include "libbyz.h"
#include "Set.h"

#include "Request_history.h"

#include "T_Certificate.h"
#include "T_Ack.h"
#include "T_CommitAck.h"
#include "T_Commit.h"
#include "T_Request.h"

#include <vector>

#define ALIGNMENT_BYTES 2

class T_Replica : public T_Node
{
	public:

		T_Replica(FILE *config_file, FILE *config_priv, char *host_name, short port=0);
		// Effects: Create a new server replica using the information in
		// "config_file".

		virtual ~T_Replica();
		// Effects: Kill server replica and deallocate associated storage.

		void register_exec(int (*e)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool));
		// Effects: Registers "e" as the exec_command function.

		template <class T> inline void gen_handle(T_Message *m, int sock)
		{
			T *n;
			if (T::convert(m, n))
			{
				handle(n, sock);
			} else
			{
				delete m;
			}
		}

	private:
		void processRequest(T_Request *m);
		void handle(T_Request *m, int sock);
		void handle(T_Ack *m, int sock);
		void handle(T_CommitAck *m, int sock);
		void handle(T_Commit *m, int sock);
		// Effects: Handles the checkpoint message

		Seqno seqno; // Sequence number to attribute to next protocol message,
			     // we'll use this counter to index messages in history 

		int (*exec_command)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool);

		bool execute_request(T_Request *m);

		void join_mcast_group();
		// Effects: Enables receipt of messages sent to replica group

		void leave_mcast_group();
		// Effects: Disables receipt of messages sent to replica group

		// Last replies sent to each principal.x
		T_Rep_info *replies;

		// Request history
		//Req_history_log *rh;

		//std::vector<T_Request*> bufferedRequests; //to keep out of order received T_Requests
		void processReceivedMessage(T_Message* msg, int sock);

		T_Certificate<T_Ack> r_acks; // Certificate with request acks
		T_Certificate<T_CommitAck> r_cacks; // Certificate with commit acks
		T_Request** outstandingMsg;
		std::vector<int> outstandingMsgIndex;
		int servicingIndex;
};

// Pointer to global replica object.
extern T_Replica *T_replica;

#endif //_T_Replica_h
