#ifndef _R_Node_h
#define _R_Node_h 1

#include <stdio.h>
#include "types.h"
#include "R_Principal.h"
#include "th_assert.h"
#include "R_Reply.h"
#include "R_Mes_queue.h"

extern "C"
{
#include "TCP_network.h"
}

class R_Message;
class R_Request;
class rabin_priv;
class New_key;
class ITimer;

extern void atimer_handler();

class R_Node
{
	public:

		R_Node(FILE *config_file, FILE *config_priv, char *host_name, short port=0);
		// Effects: Create a new R_Node object using the information in
		// "config_file".  If port is 0, use the first
		// line from configuration whose host address matches the address
		// of this host to represent this principal. Otherwise, search
		// for a line with port "port".

		virtual ~R_Node();
		// Effects: Deallocates all storage associated with node.

		int n() const;
		int f() const;

		int id() const;
		// Effects: Returns the principal identifier of the current node.

		int distance(int s, int d) const;

		R_Principal *i_to_p(int id) const;
		// Effects: Returns the principal that corresponds to
		// identifier "id" or 0 if "id" is not valid.

		bool is_replica(int id) const;
		// Effects: Returns true iff id() is the identifier of a valid replica.

		void send(R_Message *m, int i);
		// Requires: "i" is either R_All_replicas or a valid principal
		// identifier.
		// Effects: Sends an unreliable message "m" to all replicas or to
		// principal "i".

		R_Message* recv(int socket, int *status = NULL);
		// Effects: Blocks waiting to receive a message (while calling
		// handlers on expired timers) on the given socket then returns message.  The caller is
		// responsible for deallocating the message.
		// if err is not NULL, will pass the number of bytes received.
		// *status == 0 is signal of closed connection

		R_Mes_queue incoming_queue;
		pthread_mutex_t incoming_queue_mutex;
		pthread_cond_t not_empty_incoming_queue_cond;

		Request_id new_rid();
		// Effects: Computes a new request identifier. The new request
		// identifier is guaranteed to be larger than any request identifier
		// produced by the node in the past (even accross) reboots (assuming
		// clock as returned by gettimeofday retains value after a crash.)


		//
		// Cryptography:
		//
		unsigned sig_size(int id=-1) const;
		// Requires: id < 0 | id >= num_principals
		// Effects: Returns the size in bytes of a signature for principal
		// "id" (or current principal if "id" is negative.)
		int auth_size(int id=-1) const;
		int authenticators_size() const;
		int authenticators_size_after_verification() const;

		void gen_auth(char *src, unsigned src_len, char *dest, R_Reply_rep *rr,
				int principal_id, int entry_replica, int cid, int iteration, int skip = 0) const;

		bool verify_auth(int n_id, int entry_replica, int i, char *src, unsigned src_len,
				char *dest, int *authentication_offset, R_Request *r, int iteration);

		// our specialized authentication functions
		bool verify_client_auth(int n_id, int entry_replica, int cid, char *s, unsigned l, char *dest, int *authentication_offset, R_Request *r, int iteration);
		bool verify_batched_auth(int n_id, int entry_replica, int cid, char *s, unsigned l, char *dest, int *authentication_offset, R_Request *r, int iteration, int skip);

		void gen_signature(const char *src, unsigned src_len, char *sig);
		// Requires: "sig" is at least sig_size() bytes long.
		// Effects: Generates a signature "sig" (from this principal) for
		// "src_len" bytes starting at "src" and puts the result in "sig".

		unsigned decrypt(char *src, unsigned src_len, char *dst,
				unsigned dst_len);
		// Effects: decrypts the cyphertext in "src" using this
		// principal's private key and places up to "dst_len" bytes of the
		// result in "dst". Returns the number of bytes placed in "dst".

	protected:
		int node_id; // identifier of the current node.
		int max_faulty; // Maximum number of faulty replicas.
		int num_replicas; // Number of replicas in the service. It must be

		rabin_priv *priv_key; // PBFT_R_Node's private key.
		// Map from principal identifiers to R_Principal*. The first "num_replicas"
		// principals correspond to the replicas.
		R_Principal **principals;
		R_Principal *group;
		int num_principals;

		// Communication variables.
		Request_id cur_rid; // state for unique identifier generator.
		void new_tstamp();
		// Effects: Computes a new timestamp for rid.

};

inline int R_Node::auth_size(int id) const
{
	return R_UMAC_size * ((f() + 1) * (f() + 2)) / 2 + R_UNonce_size
			* (f() + 1);
}

inline int R_Node::n() const
{
	return num_replicas;
}

inline int R_Node::f() const
{
	return max_faulty;
}

inline int R_Node::id() const
{
	return node_id;
}

inline int R_Node::distance(int s, int d) const
{
	return (d-s+num_replicas)%num_replicas;
}

inline R_Principal* R_Node::i_to_p(int id) const
{
	if (id < 0|| id >= num_principals)
	{
		return 0;
	}
	return principals[id];
}

inline bool R_Node::is_replica(int id) const
{
	return id >= 0&& id < num_replicas;
}

// Pointer to global node object.
extern R_Node *R_node;
extern R_Principal *R_my_principal;

inline unsigned R_Node::sig_size(int id) const
{
	if (id < 0)
		id = node_id;
	th_assert(id < num_principals, "Invalid argument");
	return principals[id]->sig_size();
}

#endif // _R_Node_h
