#ifndef _C_Replica_h
#define _C_Replica_h 1

#include <pthread.h>

#include <map>
#include <list>

#include "C_Cryptography.h"
#include "types.h"
#include "C_Rep_info.h"
#include "Digest.h"
#include "C_Node.h"
#include "libbyz.h"
#include "Set.h"
#include "C_Checkpoint.h"

#include "Request_history.h"

class C_Request;
#ifdef REPLY_BY_PRIMARY
class C_Deliver;
#endif

typedef Set<C_Checkpoint> CheckpointSet;

class C_Checkpoint_Store {
    public:
	C_Checkpoint_Store() : by_chk_id(), by_set(), size(0), lower(0), upper(0), last_removed(0) {};
	~C_Checkpoint_Store() { for (int i=lower; i<upper; i++) delete by_set[i]; };

	void add(Seqno seq, CheckpointSet *set) {
	    by_chk_id.append(seq);
	    by_set.append(set);
	    size++;
	    upper++;
	};

	bool find(Seqno seq, CheckpointSet* set) {
	    int pos = lower;
	    set = NULL;
	    while (pos < upper) {
		if (by_chk_id[pos] == seq) {
		    set = by_set[pos];
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
	Array<CheckpointSet*> by_set;
	int size;
	int lower;
	int upper;
	Seqno last_removed;
	static const int TRUNCATE_SIZE = 16;
};

#define MAX_CONNECTIONS 850
#define ALIGNMENT_BYTES 8

class C_Replica : public C_Node
{
	public:

		C_Replica(FILE *config_file, FILE *config_priv, char* host_name,short port=0);

		// Effects: Create a new server replica using the information in
		// "config_file". The replica's state is set has
		// a total of "num_objs" objects. The abstraction function is
		// "get_segment" and its inverse is "put_segments". The procedures
		// invoked before and after recovery to save and restore extra
		// state information are "shutdown_proc" and "restart_proc".

		virtual ~C_Replica();
		// Effects: Kill server replica and deallocate associated storage.

		// Methods to register service specific functions. The expected
		// specifications for the functions are defined below.
		//void register_exec(int (*e)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool));
		// Effects: Registers "e" as the exec_command function.

		// Methods to register service specific functions. The expected
		// specifications for the functions are defined below.
		void register_exec(int (*e)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool));
		// Effects: Registers "e" as the exec_command function.

		void do_recv_from_queue();
		//   virtual void recv();
		// Effects: Loops receiving messages and calling the appropriate
		// handlers.

		void request_queue_handler();
		void requests_from_predecessor_handler();
		void c_client_requests_handler();

		template <class T> inline void gen_handle(C_Message *m)
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

	private:

		void handle(C_Message* m);
		void handle(C_Request* m);
		void handle(C_Checkpoint* m);
#ifdef REPLY_BY_PRIMARY
		void handle(C_Deliver* m);
#endif


		void handle_new_connection();

		Seqno seqno; // Sequence number to attribute to next protocol message,
			     // we'll use this counter to index messages in history 

		// Last replies sent to each principal.
		C_Rep_info replies;

		int in_socket;
		int in_socket_for_clients;
		int out_socket;
		int all_replicas_socket;

		// Threads
		pthread_t request_queue_handler_thread;
		pthread_t requests_from_predecessor_handler_thread;
		pthread_t C_client_requests_handler_thread;

		/* Array of connected sockets so we know who we are talking to */
		int connectlist[MAX_CONNECTIONS];

		// Pointers to the function to be executed when receiving a message
		int (*exec_command)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool);

		// request history
		Req_history_log rh;

		// checkpoint store
		C_Checkpoint_Store checkpoint_store;

#ifdef REPLY_BY_PRIMARY
		std::map<int,C_Request*> tosend;
#endif
};

// Pointer to global replica object.
extern C_Replica *C_replica;

#endif //_C_Replica_h
