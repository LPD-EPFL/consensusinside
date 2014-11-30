#ifndef _R_ACK_h
#define _R_ACK_h 1

#include "R_Message.h"
#include "types.h"
#include "R_Reply.h"
#include "ARequest.h"

class Digest;
class R_Principal;

//
// R_ACK messages have the following format.
//
struct R_ACK_rep: public R_Message_rep {
	short cid; // unique id of client who sends the request
	// short is_init;
	//short unused;
	//int unused1;
	Request_id rid; // unique request identifier

	uint32_t type;
	Digest od; // Digest from R_Request.
	Seqno seqno; // sequence number given by the sequencer
	// Followed by authenticator for R_Request
	// Followed by authenticator for this message
	// Digest resp_digest;
};

class R_ACK: public R_Message {
	//
	// R_ACK messages:
	//
	// Requires: Requests that may have been allocated by library users
	// through the libbyz.h interface can not be trimmed (this could free
	// memory the user expects to be able to use.)
	//
public:
	R_ACK();
	R_ACK(R_ACK_rep *rep);
	R_ACK(int cid, Seqno& seqno, Request_id& r, Digest& d);
	// Effects: Creates a new R_ACK message

	~R_ACK();

	static const int big_req_thresh = 0; // Maximum size of not-big requests

	int client_id() const;
	// Effects: Fetches the identifier of the client from the message.

	Request_id& request_id();
	// Effects: Fetches the request identifier from the message.

	Seqno seqno() const { return rep().seqno; };

	void set_batched();
	// Effects: Sets request to be type batched

	bool is_batched() const;
	// Effects: Returns true if the message is batched.

	void set_ack_batched();
	// Effects: Sets request to be type ack batched, ie just acks were batched

	bool is_ack_batched() const;
	// Effects: Returns true if the message is batched.

	static bool convert(R_Message *m1, R_ACK *&m2);
	// Effects: If "m1" has the right size and tag of a "R_ACK",
	// casts "m1" to a "R_ACK" pointer, returns the pointer in
	// "m2" and returns true. Otherwise, it returns false.
	Digest& digest() const {
		return rep().od;
	}

	void set_digest(Digest& d);
	// Effects: sets the digest

private:
	R_ACK_rep &rep() const;
	// Effects: Casts "msg" to a R_ACK_rep&

	class IteratorData {
	    public:
	    R_ACK *oreq; // pointer to the request
	    R_ACK *req; // pointer to the extracted request
	    int index;
	    int offset; // offset from the beginning
	    bool operator==(const IteratorData& o) {
		return (o.oreq == oreq)&&(o.req==req)&&(o.index==index)&&(o.offset==offset); }
	};


public:
	class Iterator {
	    public:
		Iterator(IteratorData d) {
		    data = d;
		}
		~Iterator() {}
		bool operator==(const Iterator& other) { return (data==other.data); }
		bool operator!=(const Iterator& other) { return !(data==other.data); }
		Iterator& operator=(const Iterator& other) {
		    data = other.data;
		    return *this;
		}
		Iterator& operator++();
	    //private:
		IteratorData data;
	};
	Iterator begin() {
	    IteratorData dt;
	    dt.oreq = dt.req = this;
	    dt.index = 0;
	    dt.offset = 0;
	    return Iterator(dt);
	}
	Iterator end() {
	    IteratorData dt;
	    dt.oreq = this;
	    dt.req = NULL;
	    dt.index = -1;
	    dt.offset = -1;
	    return Iterator(dt);
	}
};

inline R_ACK_rep& R_ACK::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((R_ACK_rep*) msg);
}

inline int R_ACK::client_id() const {

	return rep().cid;
}

inline Request_id &R_ACK::request_id() {
	return rep().rid;
}

inline void R_ACK::set_batched() {
    rep().extra = rep().extra | R_MESSAGE_BATCH;
}

inline bool R_ACK::is_batched() const {
    return rep().extra & R_MESSAGE_BATCH;
}

inline void R_ACK::set_ack_batched() {
    rep().extra = rep().extra | R_MESSAGE_ACK_BATCH;
}

inline bool R_ACK::is_ack_batched() const {
    return rep().extra & R_MESSAGE_ACK_BATCH;
}

#endif // _R_ACK_h
