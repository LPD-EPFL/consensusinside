#ifndef _R_Deliver_h
#define _R_Deliver_h 1

#include "C_Message.h"
#include "types.h"
#include "C_Reply.h"
#include "ARequest.h"

class Digest;
class C_Principal;

//
// C_Deliver messages have the following format.
//
struct C_Deliver_rep: public C_Message_rep {
	short cid; // unique id of client who sends the request
	// short is_init;
	//short unused;
	//int unused1;
	Request_id rid; // unique request identifier

	char type; // can be 0 or 1. 0 is when doing regular C_Request, 1 when working on C_Deliver
	Digest od; // Digest from C_Request.
	// Followed by authenticator for C_Request
	// Followed by authenticator for this message
	// Digest resp_digest;
};

class C_Deliver: public C_Message {
	//
	// C_Deliver messages:
	//
	// Requires: Requests that may have been allocated by library users
	// through the libbyz.h interface can not be trimmed (this could free
	// memory the user expects to be able to use.)
	//
public:
	C_Deliver();
	C_Deliver(int cid, Request_id& r, Digest& d);
	// Effects: Creates a new C_Deliver message

	~C_Deliver();

	static const int big_req_thresh = 0; // Maximum size of not-big requests

	int client_id() const;
	// Effects: Fetches the identifier of the client from the message.

	Request_id& request_id();
	// Effects: Fetches the request identifier from the message.

	static bool convert(C_Message *m1, C_Deliver *&m2);
	// Effects: If "m1" has the right size and tag of a "C_Deliver",
	// casts "m1" to a "C_Deliver" pointer, returns the pointer in
	// "m2" and returns true. Otherwise, it returns false.
	Digest& digest() const {
		return rep().od;
	}

	void set_digest(Digest& d);
	// Effects: sets the digest

private:
	C_Deliver_rep &rep() const;
	// Effects: Casts "msg" to a C_Deliver_rep&
};

inline C_Deliver_rep& C_Deliver::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((C_Deliver_rep*) msg);
}

inline int C_Deliver::client_id() const {

	return rep().cid;
}

inline Request_id &C_Deliver::request_id() {
	return rep().rid;
}

#endif // _R_Deliver_h
