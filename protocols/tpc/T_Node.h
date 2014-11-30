#ifndef _T_Node_h
#define _T_Node_h 1

#include <stdio.h>
#include <pthread.h>
#include "types.h"
#include "libbyz.h"
#include "T_Principal.h"
#include "th_assert.h"

class T_Message;
class ITimer;
class rabin_priv;

class T_Node {
public:
	static const int T_All_replicas = -1;

	T_Node(FILE *config_file, FILE *config_priv, char *host_name, short port = 0);
	// Effects: Create a new Node object using the information in
	// "config_file".  If port is 0, use the first
	// line from configuration whose host address matches the address
	// of this host to represent this principal. Otherwise, search
	// for a line with port "port".

	virtual ~T_Node();
	// Effects: Deallocates all storage associated with node.

	int n() const;
	// Effects: Returns the number of replicas
	int f() const;
	// Effects: Returns the max number of faulty replicas
	int id() const;
	// Effects: Returns the principal identifier of the current node.
	
	//Added by Maysam Yabandeh
	int primary() const;
	// Returns the id of the primary

	T_Principal *i_to_p(int id) const;
	// Effects: Returns the principal that corresponds to
	// identifier "id" or 0 if "id" is not valid.

	bool is_replica(int id) const;
	// Effects: Returns true iff id() is the identifier of a valid replica.

	enum protocols_e instance_id() const;
	// Effects: Returns the id of the instance (tpc)


	//
	// Communication methods:
	//

	void send(T_Message *m, int i);
	//Added by Maysam Yabandeh
	void sendToSocket(T_Message *m, int i);
	// Requires: "i" is either T_All_replicas or a valid principal
	// identifier.
	// Effects: Sends an unreliable message "m" to all replicas or to
	// principal "i".

	T_Message* recv();
	// Effects: Blocks waiting to receive a message (while calling
	// handlers on expired timers) then returns message.  The caller is
	// responsible for deallocating the message.

	bool has_messages(long to);
	// Effects: Call handles on expired timers and returns true if
	// there are messages pending. It blocks to usecs waiting for messages

	//
	// Cryptography:
	//
	unsigned sig_size(int id=-1) const;

	unsigned auth_size(int id = -1) const;
	// Effects: Returns the size in bytes of an authenticator for principal
	// "id" (or current principal if "id" is negative.)
	// Unique identifier generation:
	//

	Request_id new_rid();
	// Effects: Computes a new request identifier. The new request
	// identifier is guaranteed to be larger than any request identifier
	// produced by the node in the past (even accross) reboots (assuming
	// clock as returned by gettimeofday retains value after a crash.)

protected:
	int node_id; // identifier of the current node.
	int max_faulty; // Maximum number of faulty replicas.
	int num_replicas; // Number of replicas in the service. It must be

	//rabin_priv *priv_key; // T_Node's private key.
	// Map from principal identifiers to Principal*. The first "num_replicas"
	// principals correspond to the replicas.
	T_Principal **principals;
	int num_principals;

	// Special principal associated with the group of replicas.
	T_Principal *group;

	// Communication variables.
	int sock;
	Request_id cur_rid; // state for unique identifier generator.
	void new_tstamp();
	// Effects: Computes a new timestamp for rid.


	//Added by Maysam Yabandeh
	void serverListen();
	virtual void processReceivedMessage(T_Message* m, int sock) = 0;
	friend void T_recvtask(void*);
};

inline int T_Node::n() const {
	return num_replicas;
}

inline int T_Node::f() const {
	return max_faulty;
}

inline int T_Node::id() const {
	return node_id;
}

inline T_Principal* T_Node::i_to_p(int id) const {
	if (id < 0 || id >= num_principals) {
		return 0;
	}
	return principals[id];
}

inline bool T_Node::is_replica(int id) const {
	return id >= 0 && id < num_replicas;
}

inline unsigned T_Node::auth_size(int id) const {
	//if (id < 0) {
		//id = node_id;
	//}
	//return ((id < num_replicas) ? num_replicas - 1 : num_replicas) * T_UMAC_size
			//+ T_UNonce_size;
	return 0;
}

inline unsigned T_Node::sig_size(int id) const
{
	//if (id < 0)
		//id = node_id;
	//th_assert(id < num_principals, "Invalid argument");
	//return principals[id]->sig_size();
	return 0;
}

inline enum protocols_e T_Node::instance_id() const
{
    return tpc;
}


//Added by Maysam Yabandeh
enum
{
	STACK = 32768
};
inline int T_Node::primary() const {return 0;}
void T_recvtask(void*);

// Pointer to global node object.
extern T_Node *T_node;

extern T_Principal *T_my_principal;

#endif // _T_Node_h
