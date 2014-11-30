#ifndef _R_Deliver_h
#define _R_Deliver_h 1

#include "R_Message.h"
#include "types.h"
#include "R_Reply.h"
#include "ARequest.h"

class Digest;
class R_Principal;

//
// R_Deliver messages have the following format.
//
struct R_Deliver_rep: public R_Message_rep {
	short cid; // unique id of client who sends the request
	// short is_init;
	//short unused;
	//int unused1;
	Request_id rid; // unique request identifier

	char type; // can be 0 or 1. 0 is when doing regular R_Request, 1 when working on R_Deliver
	Digest od; // Digest from R_Request.
	Seqno seqno; // sequence number given by the sequencer
	// Followed by authenticator for R_Request
	// Followed by authenticator for this message
	// Digest resp_digest;
};

class R_Deliver: public R_Message {
	//
	// R_Deliver messages:
	//
	// Requires: Requests that may have been allocated by library users
	// through the libbyz.h interface can not be trimmed (this could free
	// memory the user expects to be able to use.)
	//
public:
	R_Deliver();
	R_Deliver(int cid, Seqno& seqno, Request_id& r, Digest& d);
	// Effects: Creates a new R_Deliver message

	~R_Deliver();

	static const int big_req_thresh = 0; // Maximum size of not-big requests

	int client_id() const;
	// Effects: Fetches the identifier of the client from the message.

	Request_id& request_id();
	// Effects: Fetches the request identifier from the message.

	Seqno seqno() const { return rep().seqno; };

	static bool convert(R_Message *m1, R_Deliver *&m2);
	// Effects: If "m1" has the right size and tag of a "R_Deliver",
	// casts "m1" to a "R_Deliver" pointer, returns the pointer in
	// "m2" and returns true. Otherwise, it returns false.
	Digest& digest() const {
		return rep().od;
	}

	void set_digest(Digest& d);
	// Effects: sets the digest

private:
	R_Deliver_rep &rep() const;
	// Effects: Casts "msg" to a R_Deliver_rep&
};

inline R_Deliver_rep& R_Deliver::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((R_Deliver_rep*) msg);
}

inline int R_Deliver::client_id() const {

	return rep().cid;
}

inline Request_id &R_Deliver::request_id() {
	return rep().rid;
}

#endif // _R_Deliver_h
