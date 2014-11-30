#ifndef _C_Request_h
#define _C_Request_h 1

#include "C_Message.h"
#include "types.h"
#include "C_Reply.h"
#include "ARequest.h"

class Digest;
class C_Principal;

//
// C_Request messages have the following format.
//
struct C_Request_rep: public C_Message_rep {
	Digest od; // Digest of rid, cid, command.
	short replier; // id of replica from which client
	// expects to receive a full reply
	// (if negative, it means all replicas).
	short command_size;
	short cid; // unique id of client who sends the request
	// short is_init;
	//short unused;
	//int unused1;
	Request_id rid; // unique request identifier
	// Followed a command which is "command_size" bytes long and an
	// authenticator.
	// Digest resp_digest;
};

class C_Request: public C_Message {
	//
	// C_Request messages:
	//
	// Requires: Requests that may have been allocated by library users
	// through the libbyz.h interface can not be trimmed (this could free
	// memory the user expects to be able to use.)
	//
public:
	C_Request();
	C_Request(Request_id r, short rr = -1);
	// Effects: Creates a new C_Request message with an empty
	// command and no authentication. The methods store_command and
	// authenticate should be used to finish message construction.
	// "rr" is the identifier of the replica from which the client
	// expects a full reply (if negative, client expects a full reply
	// from all replicas).

	~C_Request();

	static const int big_req_thresh = 0; // Maximum size of not-big requests
	char* store_command(int &max_len);
	// Effects: Returns a pointer to the location within the message
	// where the command should be stored and sets "max_len" to the number of
	// bytes available to store the reply. The caller can copy any command
	// with length less than "max_len" into the returned buffer.

	void finalize(int act_len, bool read_only);

	void authenticate(int id, int authentication_offset, C_Reply_rep *rr);
	// Effects: Terminates the construction of a request message by
	// setting the length of the command to "act_len", and appending an
	// authenticator. read-only should be true iff the request is read-only
	// (i.e., it will not change the service state).

	int copy_authenticators(char *dest);
	// Copies the authenticator to dest
	void replace_authenticators(char *src);

	bool verify(int *authentication_offset);

	int client_id() const;
	// Effects: Fetches the identifier of the client from the message.

	Request_id& request_id();
	// Effects: Fetches the request identifier from the message.

	char* command(int &len);
	// Effects: Returns a pointer to the command and sets len to the
	// command size.

	char *MACs();

	//Digest& digest() const;
	// Effects: Returns the digest of the string obtained by
	// concatenating the client_id, the request_id, and the command.

	int replier() const;
	// Effects: Returns the identifier of the replica from which
	// the client expects a full reply. If negative, client expects
	// a full reply from all replicas.

	bool is_read_only() const;
	// Effects: Returns true iff the request message states that the
	// request is read-only.

	static bool convert(C_Message *m1, C_Request *&m2);
	// Effects: If "m1" has the right size and tag of a "C_Request",
	// casts "m1" to a "C_Request" pointer, returns the pointer in
	// "m2" and returns true. Otherwise, it returns false.
	Digest& digest() const {
		return rep().od;
	}

private:
	C_Request_rep &rep() const;
	// Effects: Casts "msg" to a C_Request_rep&

	void comp_digest(Digest& d);
	// Effects: computes the digest of rid, cid, and the command.
};

inline C_Request_rep& C_Request::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((C_Request_rep*) msg);
}

inline int C_Request::client_id() const {

	return rep().cid;
}

inline Request_id &C_Request::request_id() {
	return rep().rid;
}

inline char *C_Request::command(int &len) {
	len = rep().command_size;
	return contents() + sizeof(C_Request_rep);
}

inline int C_Request::replier() const {
	return rep().replier;
}

inline bool C_Request::is_read_only() const {
	// return false;
	return rep().extra & 1;
}

inline char *C_Request::MACs() {
	return contents() + sizeof(C_Request_rep) + rep().command_size;
}
#endif // _C_Request_h
