#ifndef _R_Replica_h
#define _R_Replica_h 1

#include <pthread.h>

#include <google/sparse_hash_map>
#include <google/dense_hash_map>

using google::dense_hash_map;

#include <map>
#include <list>

#include "R_Cryptography.h"
#include "types.h"
#include "R_Rep_info.h"
#include "Digest.h"
#include "R_Node.h"
#include "libbyz.h"
#include "Set.h"
#include "Array.h"
#include "R_Checkpoint.h"

#include "SuperFastHash.h"

class R_Request;
class R_ACK;
#ifdef REPLY_BY_PRIMARY
class R_Deliver;
#endif

#define MAX_CONNECTIONS 200
#define ALIGNMENT_BYTES 2

enum R_State { state_pending = 0, state_exec_not_acked = 1, state_acked = 2, state_wait_deliver = 4, state_stable = 8 };
enum Execution { execution_ok = 0, execution_wait = 1, execution_old = 2 }; // wait means still not executed, old means request has been executed previously. ok means it was executed now.

typedef struct {
    R_Request *req;
    enum R_State state;
    int	authentication_offset;
} R_Message_State;

typedef std::pair<int, Request_id> R_Message_Id;
typedef std::list<R_Message_State> R_Message_List;
//typedef google::dense_hash_map<R_Message_Id, R_Message_State, SuperFastHasher<R_Message_Id> > R_Message_Map;
typedef std::map<R_Message_Id, R_Message_State> R_Message_Map;

class R_Replica : public R_Node
{
	public:

		R_Replica(FILE *config_file, FILE *config_priv, char* host_name,short port=0);

		// Effects: Create a new server replica using the information in
		// "config_file". The replica's state is set has
		// a total of "num_objs" objects. The abstraction function is
		// "get_segment" and its inverse is "put_segments". The procedures
		// invoked before and after recovery to save and restore extra
		// state information are "shutdown_proc" and "restart_proc".

		virtual ~R_Replica();
		// Effects: Kill server replica and deallocate associated storage.

		// Methods to register service specific functions. The expected
		// specifications for the functions are defined below.
		//void register_exec(int (*e)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool));
		// Effects: Registers "e" as the exec_command function.

		// Methods to register service specific functions. The expected
		// specifications for the functions are defined below.
		void register_exec(int (*e)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool));
		// Effects: Registers "e" as the exec_command function.

		Execution execute_request(R_Request *req);
		void execute_request_main(R_Request *req);
		void send_reply_to_client(R_Request *req);

		void do_recv_from_queue();
		//   virtual void recv();
		// Effects: Loops receiving messages and calling the appropriate
		// handlers.

		void request_queue_handler();
		void requests_from_predecessor_handler();
		void c_client_requests_handler();

		template <class T> inline void gen_handle(R_Message *m)
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

		void handle(R_Message* m);
		void handle(R_Request* m);
		void handle(R_ACK * m);
		void handle(R_Checkpoint* m);
#ifdef REPLY_BY_PRIMARY
		void handle(R_Deliver* m);
#endif

		void handle_new_connection();

		Seqno seqno; // Sequence number to attribute to next protocol message,
			     // we'll use this counter to index messages in history 
		Seqno last_executed;

		// Last replies sent to each principal.
		R_Rep_info replies;

		int in_socket;
		int in_socket_for_clients;
		int out_socket;
		int all_replicas_socket;

		// Threads
		pthread_t request_queue_handler_thread;
		pthread_t requests_from_predecessor_handler_thread;
		pthread_t R_client_requests_handler_thread;

		// Queues for holding messages from clients
		R_Mes_queue incoming_queue_clients;
		int incoming_queue_signaler[2];

		/* Array of connected sockets so we know who we are talking to */
		google::dense_hash_map<int,int> connectmap;

#ifdef REPLY_BY_PRIMARY
		std::map<int,R_Request*> tosend;
#endif

		// Pointers to the function to be executed when receiving a message
		int (*exec_command)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool);

		// Logical vector clocks
		Array<Seqno> vector_clock;

		R_Message_Map	requests;
};

// Pointer to global replica object.
extern R_Replica *R_replica;

#endif //_R_Replica_h
