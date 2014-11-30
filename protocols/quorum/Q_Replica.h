#ifndef _Q_Replica_h
#define _Q_Replica_h 1

#include <pthread.h>

//#include "Q_Cryptography.h"
#include "types.h"
#include "Q_Rep_info.h"
#include "Digest.h"
#include "Q_Node.h"
#include "libbyz.h"
#include "Set.h"
#include "Q_Checkpoint.h"
#include "Q_Panic.h"
#include "Q_Abort.h"
#include "Q_Abort_certificate.h"
#include "Q_Smasher.h"
#include "Q_ITimer.h"

#include "Switcher.h"

#include "Request_history.h"

#include <list>

//Added by Maysam Yabandeh
#include <vector>

class Q_Request;
class Q_Reply;
class Q_Missing;
class Q_Get_a_grip;

#define ALIGNMENT_BYTES 2

typedef Set<Q_Checkpoint> Q_CheckpointSet;


extern void Q_abort_timeout_handler();


class Q_Checkpoint_Store {
    public:
	~Q_Checkpoint_Store() { for (int i=lower; i<upper; i++) delete by_set[i]; };
	Q_Checkpoint_Store() : by_chk_id(TRUNCATE_SIZE), by_set(TRUNCATE_SIZE), size(0), lower(0), upper(0), last_removed(0) {};

	void clear_all() {
	    for (int i=lower; i<upper; i++)
		delete by_set[i];
	    lower=0;
	    upper=0;
	    size=0;
	    last_removed=0;
	    by_chk_id.enlarge_to(TRUNCATE_SIZE);
	    by_set.enlarge_to(TRUNCATE_SIZE);
	    by_chk_id.clear();
	    by_set.clear();
	};

	void add(Seqno seq, Q_CheckpointSet *set) {
	    by_chk_id.append(seq);
	    by_set.append(set);
	    size++;
	    upper++;
	};

	bool find(Seqno seq, Q_CheckpointSet** set) {
	    int pos = lower;
	    *set = NULL;
	    while (pos < upper) {
		if (by_chk_id[pos] == seq) {
		    *set = by_set[pos];
		    return true;
		}
		pos++;
	    }
	    return false;
	};

	void remove(Seqno seq) {
	    int pos = lower;
	    while (pos < upper) {
		if (by_chk_id[pos] == seq) {
		    last_removed = seq;
		    size -= (pos-lower+1);
		    int oldlower = lower;
		    lower = pos+1;
		    if (lower > TRUNCATE_SIZE) {
		    	for (int i=oldlower; i<lower; i++)
			    if (by_set[i] != NULL)
				delete by_set[i];

			// need to truncate the file
			for (int i=0; i<size; i++) {
			    by_chk_id[i] = by_chk_id[i+lower];
			    by_set[i] = by_set[i+lower];
			}
			upper -= lower;
			lower = 0;
			by_chk_id.enlarge_to(size);
			by_set.enlarge_to(size);
		    }
		    return;
		}
		pos++;
	    }
	}

	Seqno last() const { return last_removed; }

    private:
	Array<Seqno> by_chk_id;
	Array<Q_CheckpointSet*> by_set;
	int size;
	int lower;
	int upper;
	Seqno last_removed;
	static const int TRUNCATE_SIZE = 128;
};

class Q_Replica : public Q_Node
{
	public:

		Q_Replica(FILE *config_file, FILE *config_priv, char *host_name, short port=0);
		// Effects: Create a new server replica using the information in
		// "config_file".

		virtual ~Q_Replica();
		// Effects: Kill server replica and deallocate associated storage.

		void register_exec(int (*e)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool));
		// Effects: Registers "e" as the exec_command function.

		void register_perform_checkpoint(int (*p)());
		// Effects: Registers "p" as the perform_checkpoint function.

		void handle_incoming_messages();
		// Effects: Loops receiving messages and calling the appropriate
		// handlers.

		template <class T> inline void gen_handle(Q_Message *m, int sock)
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

		void enable_replica(bool state);
		// if state == true, enable, else disable

		void retransmit_panic();

	private:
		void handle(Q_Request *m, int sock);
		void handle(Q_Checkpoint *c, int sock);
		void handle(Q_Panic *c, int sock);
		void handle(Q_Abort *m, int sock);
		void handle(Q_Missing *m, int sock);
		void handle(Q_Get_a_grip *m, int sock);
		// Effects: Handles the checkpoint message

		Seqno seqno; // Sequence number to attribute to next protocol message,
			     // we'll use this counter to index messages in history 

		int (*exec_command)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool);

		int (*perform_checkpoint)();

		bool execute_request(Q_Request *m);

		void broadcast_abort(Request_id out_rid);
		// Effects: Sends abort message to all

		void join_mcast_group();
		// Effects: Enables receipt of messages sent to replica group

		void leave_mcast_group();
		// Effects: Disables receipt of messages sent to replica group

		void notify_outstanding();
		// Effects: notifies all outstanding clients to abort

		// Last replies sent to each principal.x
		Q_Rep_info *replies;

		// Used to generate the id of the next replier
		int replier;
		int nb_retransmissions;

		// Request history
		Req_history_log *rh;

		// checkpoint store
		Q_Checkpoint_Store checkpoint_store;

		// keeps replica's state
		enum replica_state cur_state;

		// for keeping received aborts
		Q_Abort_certificate aborts;
		// keep the list of missing request
		AbortHistory *ah_2;
		AbortHistory *missing;
		// missing mask keeps the track of requests we're still missing
		// while num_missing is represents how many requests we still need
		// invariant: num_missing = sum(missing_mask[i]==true);
		unsigned int num_missing;
		Array<bool> missing_mask;
		Array<Q_Request*> missing_store;
		Array<Seqno> missing_store_seq;

		// timouts
		int n_retrans;        // Number of retransmissions of out_req
		int rtimeout;         // Q_Timeout period in msecs
		Q_ITimer *rtimer;       // Retransmission timer

		std::list<OutstandingRequests> outstanding;

		//Added by Maysam Yabandeh
		int totalOrder;
		std::vector<Q_Request*> bufferedRequests; //to keep out of order received Q_Requests
		void processReceivedMessage(Q_Message* msg, int sock);
};

// Pointer to global replica object.
extern Q_Replica *Q_replica;

#endif //_Q_Replica_h
