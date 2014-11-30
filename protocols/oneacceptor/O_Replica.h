#ifndef _O_Replica_h
#define _O_Replica_h 1

#include <pthread.h>

//#include "O_Cryptography.h"
#include "types.h"
#include "O_Rep_info.h"
//#include "Digest.h"
#include "O_Node.h"
#include "libbyz.h"
#include "Set.h"

#include "Request_history.h"

#include "O_Certificate.h"
#include "O_Request.h"
#include "O_Prepare.h"
#include "O_PrepareResponse.h"
#include "O_Abandon.h"

#include <vector>

#define ALIGNMENT_BYTES 2

class O_Replica : public O_Node
{
	public:

		O_Replica(FILE *config_file, FILE *config_priv, char *host_name, short port=0);
		// Effects: Create a new server replica using the information in
		// "config_file".

		virtual ~O_Replica();
		// Effects: Kill server replica and deallocate associated storage.

		void register_exec(int (*e)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool));
		// Effects: Registers "e" as the exec_command function.

		template <class T> inline void gen_handle(O_Message *m, int sock)
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
		void processRequest(O_Request *m);
		void handle(O_Request *m, int sock);
		void handle(O_Accept *m, int sock);
		void handle(O_Learn *m, int sock);
		void handle(O_Prepare *m, int sock);
		void handle(O_PrepareResponse *m, int sock);
		void handle(O_Abandon *m, int sock);
		// Effects: Handles the checkpoint message

		Seqno seqno; // Sequence number to attribute to next protocol message,
			     // we'll use this counter to index messages in history 

		int (*exec_command)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool);

		bool execute_request(O_Learn *m);

		void join_mcast_group();
		// Effects: Enables receipt of messages sent to replica group

		void leave_mcast_group();
		// Effects: Disables receipt of messages sent to replica group

		// Last replies sent to each principal.x
		O_Rep_info *replies;

		// Request history
		//Req_history_log *rh;

		//std::vector<O_Request*> bufferedRequests; //to keep out of order received O_Requests
		void processReceivedMessage(O_Message* msg, int sock);

      map<uint64_t, O_Request*> proposals;
      map<uint64_t, O_Accept*> accepted;
      map<uint64_t, O_Learn*> chosenValues;
		bool IamLeader;//If I think that I am the leader
		int activeAcceptor; // the address of the active acceptor which I am using
		seq_t highest_proposal_number;//the smallest possible value for proposal number
		uint64_t next_uncommited_instance_number_var;

		void update_next_uncommited_instance_number(uint64_t commitedInstance);
		uint64_t next_uncommited_instance_number();
		void registerProposals(AcceptedProposals& acceptedProposals);
		std::vector<Proposal> acceptedProposals;//list of the proposals accepted by the others and must be checked by me
		void propose(uint64_t index, O_Request *m);
		seq_t globaln;
		seq_t getNextSeq();
		time_t lastTimeIgaveupLeadership;
		bool waitingForProposalChange;
};

// Pointer to global replica object.
extern O_Replica *O_replica;

#endif //_O_Replica_h
