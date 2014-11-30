#ifndef _PBFT_R_Node_h
#define _PBFT_R_Node_h 1

#include <stdio.h>
#include "types.h"
#include "PBFT_R_Principal.h"
#include "PBFT_R_ITimer.h"
#include "PBFT_R_Request.h"
#include "th_assert.h"
#include "libbyz.h"

#include "PBFT_R_Statistics.h"

class PBFT_R_Message;
class rabin_priv;
class PBFT_R_New_key;
class PBFT_R_ITimer;

extern void atimePBFT_R_handler();

class PBFT_R_Node
{
	public:
		PBFT_R_Node(FILE *config_file, FILE *config_priv, char *host_name,
				short port=0);
		// Effects: Create a new PBFT_R_Node object using the information in
		// "config_file" and "config_priv".  If port is 0, use the first
		// line from configuration whose host address matches the address
		// of this host to represent this principal. Otherwise, search
		// for a line with port "port".

		virtual ~PBFT_R_Node();
		// Effects: Deallocates all storage associated with node.

		View view() const;
		// Effects: Returns the last view known to this node.

		int n() const;
		int f() const;
		int n_f() const;
		int np() const;

		int id() const;
		// Effects: Returns the principal identifier of the current node.

		PBFT_R_Principal *i_to_p(int id) const;
		// Effects: Returns the principal that corresponds to
		// identifier "id" or 0 if "id" is not valid.

		PBFT_R_Principal *principal() const;
		// Effects: Returns a pointer to the principal identifier associated
		// with the current node.

		bool is_PBFT_R_replica(int id) const;
		// Effects: Returns true iff id() is the identifier of a valid PBFT_R_replica.

		enum protocols_e instance_id() const;
		// Effects: Returns the id of the instance (pbft)

		int primary(View vi) const;
		// Effects: Returns the identifier of the primary for view v.

		inline int primary() const;
		// Effects: Returns  the identifier of the primary for current view.

		//
		// Communication methods:
		//
		static const int All_PBFT_R_replicas = -1;
		void send(PBFT_R_Message *m, int i);
		// Requires: "i" is either All_PBFT_R_replicas or a valid principal
		// identifier.
		// Effects: Sends an unreliable message "m" to all PBFT_R_replicas or to
		// principal "i".

		PBFT_R_Message* recv();
		// Effects: Blocks waiting to receive a message (while calling
		// handlers on expired timers) then returns message.  The caller is
		// responsible for deallocating the message.

		bool has_messages(long to);
		// Effects: Call handles on expired timers and returns true if
		// there are messages pending. It blocks to usecs waiting for messages

		// this socket is used to sneak a message into protocol
		// with it, one can use PBFT just as a group communication protocol
		// Replica listens on this socket, but never sends a result back
		int hidden_client_sock;

		//
		// Cryptography:
		//

		//
		// Authenticator generation and verification:
		//
		int auth_size(int id=-1) const;
		// Effects: Returns the size in bytes of an authenticator for principal
		// "id" (or current principal if "id" is negative.)

		void gen_auth_out(char *src, unsigned src_len, char *dest=0) const;
		// Requires: "src" points to a string of length at least "src_len"
		// bytes. If "dest" == 0, "src+src_len" must have size >= "sig_size()";
		// otherwise, "dest" points to a string of length at least "sig_size()".
		// Effects: Computes an authenticator of "src_len" bytes
		// starting at "src" (using out-keys for principals) and places the result in
		// "src"+"src_len" (if "dest" == 0) or "dest" (otherwise).

		void gen_auth_in(char *src, unsigned src_len, char *dest=0) const;
		// Requires: same as gen_auth
		// Effects: Same as gen_auth but authenticator is computed using
		// in-keys for principals

		bool
				verify_auth_in(int i, char *src, unsigned src_len, char *dest=0) const;
		// Requires: "i" is not the calling principal and same as gen_auth
		// Effects: If "i" is an invalid principal identifier or is the
		// identifier of the calling principal, returns false and does
		// nothing. Otherwise, returns true iff: "src"+"src_len" or ("dest"
		// if non-zero) contains an authenticator by principal "i" that is
		// valid for the calling principal (i.e. computed with calling
		// principal's in-key.)

		bool
				verify_auth_out(int i, char *src, unsigned src_len,
						char *dest=0) const;
		// Requires: same as verify_auth
		// Effects: same as verify_auth except that checks an authenticator
		// computed with calling principal's out-key.)

		//
		// Signature generation:
		//
		unsigned sig_size(int id=-1) const;
		// Requires: id < 0 | id >= num_principals
		// Effects: Returns the size in bytes of a signature for principal
		// "id" (or current principal if "id" is negative.)

		void gen_signature(const char *src, unsigned src_len, char *sig);
		// Requires: "sig" is at least sig_size() bytes long.
		// Effects: Generates a signature "sig" (from this principal) for
		// "src_len" bytes starting at "src" and puts the result in "sig".

		unsigned decrypt(char *src, unsigned src_len, char *dst,
				unsigned dst_len);
		// Effects: decrypts the cyphertext in "src" using this
		// principal's private key and places up to "dst_len" bytes of the
		// result in "dst". Returns the number of bytes placed in "dst".

		//
		// Unique identifier generation:
		//
		Request_id new_rid();
		// Effects: Computes a new request identifier. The new request
		// identifier is guaranteed to be larger than any request identifier
		// produced by the node in the past (even accross) reboots (assuming
		// clock as returned by gettimeofday retains value after a crash.)

	protected:
		char service_name[256];
		int node_id; // identifier of the current node.
		int max_faulty; // Maximum number of faulty PBFT_R_replicas.
		int num_PBFT_R_replicas; // Number of PBFT_R_replicas in the service. It must be
		// num_PBFT_R_replicas == 3*max_faulty+1.
		int threshold; // Number of correct PBFT_R_replicas. It must be
		// threshold == 2*max_faulty+1.

		rabin_priv *priv_key; // PBFT_R_Node's private key.

		// Map from principal identifiers to PBFT_R_Principal*. The first "num_PBFT_R_replicas"
		// principals correspond to the PBFT_R_replicas.
		PBFT_R_Principal **principals;
		int num_principals;

		// Special principal associated with the group of PBFT_R_replicas.
		PBFT_R_Principal *group;

		View v; //  Last view known to this node.
		int cuPBFT_R_primary; // id of primary for the current view.

		//
		// Handling authentication freshness
		//
		PBFT_R_ITimer *atimer;
		friend void atimePBFT_R_handler();

		virtual void send_new_key();
		// Effects: Sends a new-key message and updates last_new_key.

		PBFT_R_New_key *last_new_key; // Last new-key message we sent.


		// Communication variables.
		int sock;

		Request_id cuPBFT_R_rid; // state for unique identifier generator.
		void new_tstamp();
		// Effects: Computes a new timestamp for rid.

		void gen_auth(char *src, unsigned src_len, bool in, char *dest) const;
		// Requires: "src" points to a string of length at least "src_len"
		// bytes. "dest" points to a string of length at least "sig_size()".
		// Effects: Computes an authenticator of "src_len" bytes starting at
		// "src" (using in-keys for principals if in is true and out-keys
		// otherwise) and places the result in "src"+"src_len" (if "dest" ==
		// 0) or "dest" (otherwise).

		bool verify_auth(int i, char *src, unsigned src_len, bool in,
				char *dest) const;
		// Requires: "i" is not the calling principal and same as gen_auth
		// Effects: If "i" is an invalid principal identifier, returns false
		// and does nothing. Otherwise, returns true iff "dest" contains an
		// authenticator by principal "i" that is valid for the calling
		// principal (i.e. computed with calling principal's in-key if in is
		// true and out-keys otherwise.)

		// set if we're using j2ee modifications
		// set to true if PBFT_J2EE env var is set to any value
		bool j2ee;
};

inline View PBFT_R_Node::view() const
{
	return v;
}

inline int PBFT_R_Node::n() const
{
	return num_PBFT_R_replicas;
}

inline int PBFT_R_Node::f() const
{
	return max_faulty;
}

inline int PBFT_R_Node::n_f() const
{
	return threshold;
}

inline int PBFT_R_Node::np() const
{
	return num_principals;
}

inline int PBFT_R_Node::id() const
{
	return node_id;
}

inline PBFT_R_Principal* PBFT_R_Node::i_to_p(int id) const
{
	if (id < 0 || id >= num_principals)
		return 0;
	return principals[id];
}

inline PBFT_R_Principal* PBFT_R_Node::principal() const
{
	return i_to_p(id());
}

inline bool PBFT_R_Node::is_PBFT_R_replica(int id) const
{
	return id >= 0 && id < num_PBFT_R_replicas;
}

inline int PBFT_R_Node::primary(View vi) const
{
	return (vi == v) ? cuPBFT_R_primary : (vi % num_PBFT_R_replicas);
}

inline int PBFT_R_Node::primary() const
{
	return cuPBFT_R_primary;
}

#ifdef USE_SECRET_SUFFIX_MD5
inline int PBFT_R_Node::auth_size(int id) const
{
	if (id < 0) id = node_id;
	return ((id < num_PBFT_R_replicas) ? num_PBFT_R_replicas - 1 : num_PBFT_R_replicas) * MAC_size;
}
#else
inline int PBFT_R_Node::auth_size(int id) const
{
	if (id < 0)
		id = node_id;
	return ((id < num_PBFT_R_replicas) ? num_PBFT_R_replicas - 1
			: num_PBFT_R_replicas) * UMAC_size + UNonce_size;
}
#endif

inline void PBFT_R_Node::gen_auth_out(char *src, unsigned src_len, char *dest) const
{
	if (dest == 0)
		dest = src+src_len;
	gen_auth(src, src_len, false, dest);
}

inline void PBFT_R_Node::gen_auth_in(char *src, unsigned src_len, char *dest) const
{
	if (dest == 0)
		dest = src+src_len;
	gen_auth(src, src_len, true, dest);
}

inline bool PBFT_R_Node::verify_auth_in(int i, char *src, unsigned src_len,
		char *dest) const
{
	if (dest == 0)
		dest = src+src_len;
	return verify_auth(i, src, src_len, true, dest);
}

inline bool PBFT_R_Node::verify_auth_out(int i, char *src, unsigned src_len,
		char *dest) const
{
	if (dest == 0)
		dest = src+src_len;
	return verify_auth(i, src, src_len, false, dest);
}

inline unsigned PBFT_R_Node::sig_size(int id) const
{
	if (id < 0)
		id = node_id;
	th_assert(id < num_principals, "Invalid argument");
	return principals[id]->sig_size();
}

inline int cyphePBFT_R_size(char *dst, unsigned dst_len)
{
	// Effects: Returns the size of the cypher in dst or 0 if dst
	// does not contain a valid cypher.
	if (dst_len < 2*sizeof(unsigned))
		return 0;

	unsigned csize;
	dst += sizeof(unsigned);
	memcpy((char*)&csize, dst, sizeof(unsigned));

	if (csize <= dst_len)
		return csize+2*sizeof(unsigned);
	else
		return 0;
}

inline enum protocols_e PBFT_R_Node::instance_id() const
{
    return pbft;
}

// Pointer to global node object.
extern PBFT_R_Node *PBFT_R_node;

#endif // _PBFT_R_Node_h
