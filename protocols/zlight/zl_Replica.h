#ifndef _zl_Replica_h
#define _zl_Replica_h 1

#include <pthread.h>

#include "zl_Cryptography.h"
#include "types.h"
#include "zl_Rep_info.h"
#include "Digest.h"
#include "zl_Node.h"
#include "libbyz.h"
#include "Set.h"
#include "zl_Checkpoint.h"
#include "zl_Panic.h"
#include "zl_Abort.h"
#include "zl_Abort_certificate.h"
#include "zl_Smasher.h"
#include "zl_ITimer.h"

#include "Switcher.h"

#include "Request_history.h"

#include <list>

class zl_Request;
class zl_Reply;
class zl_Missing;
class zl_Get_a_grip;
class zl_Order_request;

#define ALIGNMENT_BYTES 2

typedef Set<zl_Checkpoint> zl_CheckpointSet;

extern void zl_abort_timeout_handler();

class zl_Checkpoint_Store {
    public:
	~zl_Checkpoint_Store() { for (int i=lower; i<upper; i++) delete by_set[i]; };
	zl_Checkpoint_Store() : by_chk_id(TRUNCATE_SIZE), by_set(TRUNCATE_SIZE), size(0), lower(0), upper(0), last_removed(0) {};

	void add(Seqno seq, zl_CheckpointSet *set) {
	    by_chk_id.append(seq);
	    by_set.append(set);
	    size++;
	    upper++;
	};

	bool find(Seqno seq, zl_CheckpointSet** set) {
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
		    lower = pos;
		    if (lower > TRUNCATE_SIZE) {
			// need to truncate the file
			for (int i=0; i<size; i++) {
			    by_chk_id[i] = by_chk_id[i+lower];
			    if (by_set[i] != NULL)
			    	delete by_set[i];
			    by_set[i] = by_set[i+lower];
			}
			upper -= lower;
			lower = 0;
		    }
		    return;
		}
		pos++;
	    }
	}

	Seqno last() const { return last_removed; }

    private:
	Array<Seqno> by_chk_id;
	Array<zl_CheckpointSet*> by_set;
	int size;
	int lower;
	int upper;
	Seqno last_removed;
	static const int TRUNCATE_SIZE = 16;
};


class zl_Replica : public zl_Node
{
	public:

		zl_Replica(FILE *config_file, FILE *config_priv, char *host_name, short port=0);
		// Effects: Create a new server replica using the information in
		// "config_file".

		virtual ~zl_Replica();
		// Effects: Kill server replica and deallocate associated storage.

		void register_exec(int (*e)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool));
		// Effects: Registers "e" as the exec_command function.

		void register_perform_checkpoint(int (*p)());
		// Effects: Registers "p" as the perform_checkpoint function.

		void handle_incoming_messages();
		// Effects: Loops receiving messages and calling the appropriate
		// handlers.

		template <class T> inline void gen_handle(zl_Message *m)
		{
			T *n;
			if (T::convert(m, n))
			{
				handle(n);
			} else
			{
				delete m;
			}
		}

		void enable_replica(bool state);
		// if state == true, enable, else disable

		void retransmit_panic();

	private:
		void handle(zl_Request *m);
		void handle(zl_Checkpoint *c);
		void handle(zl_Panic *c);
		void handle(zl_Abort *m);
		void handle(zl_Missing *m);
		void handle(zl_Get_a_grip *m);
		void handle(zl_Order_request *m);
		// Effects: Handles the checkpoint message

		Seqno seqno; // Sequence number to attribute to next protocol message,
			     // we'll use this counter to index messages in history
		Seqno last_seqno; // Sequence number of last executed message

		int (*exec_command)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool);

		int (*perform_checkpoint)();

		bool execute_request(zl_Request *m);

		void broadcast_abort(Request_id out_rid);
		// Effects: Sends abort message to all

		void join_mcast_group();
		// Effects: Enables receipt of messages sent to replica group

		void leave_mcast_group();
		// Effects: Disables receipt of messages sent to replica group

		void notify_outstanding();
		// Effects: notifies all outstanding clients to abort

		// Last replies sent to each principal.x
		zl_Rep_info *replies;

		// Used to generate the id of the next replier
		int replier;
		int nb_retransmissions;

		// Request history
		Req_history_log *rh;

		// checkpoint store
		zl_Checkpoint_Store checkpoint_store;

		// keeps replica's state
		enum replica_state cur_state;

		// for keeping received aborts
		zl_Abort_certificate aborts;
		// keep the list of missing request
		AbortHistory *ah_2;
		AbortHistory *missing;
		// missing mask keeps the track of requests we're still missing
		// while num_missing is represents how many requests we still need
		// invariant: num_missing = sum(missing_mask[i]==true);
		unsigned int num_missing;
		Array<bool> missing_mask;
		Array<zl_Request*> missing_store;
		Array<Seqno> missing_store_seq;

		// timouts
		int n_retrans;        // Number of retransmissions of out_req
		int rtimeout;         // zl_Timeout period in msecs
		zl_ITimer *rtimer;       // Retransmission timer

		std::list<OutstandingRequests> outstanding;
};

// Pointer to global replica object.
extern zl_Replica *zl_replica;

#endif //_zl_Replica_h
