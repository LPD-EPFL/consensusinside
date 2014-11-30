#ifndef _R_Request_h
#define _R_Request_h 1

#include "R_Message.h"
#include "types.h"
#include "R_Reply.h"

class Digest;
class R_Principal;
class R_ACK;

//
// R_Request messages have the following format.
//
struct R_Request_rep: public R_Message_rep {
	// only these three will be used for calculating MAC
	uint32_t type; // can be 0 or 0xdeadbeef. 0 is when doing regular R_Request, 0xdeadbeef when working on R_ACK
	Digest od; // Digest of rid, cid, command.
	Seqno seqno; // sequence number given by the sequencer. it is non-nil if set

	int cid; // unique id of client who sends the request
	Request_id rid; // unique request identifier
	int replica_id; // id of the replica that received this request first

	short command_size;

	// Followed a command which is "command_size" bytes long,
	// possible set of batched messages, and
	// f+1 list of seqnos, and then
	// authenticator.
};

class R_Request: public R_Message {
	//
	// R_Request messages:
	//
	// Requires: Requests that may have been allocated by library users
	// through the libbyz.h interface can not be trimmed (this could free
	// memory the user expects to be able to use.)
	//
public:
	R_Request(int sz=R_Max_message_size);
	R_Request(Request_id r);
	R_Request(R_Request_rep *contents);
	// Effects: Creates a new R_Request message with an empty
	// command and no authentication. The methods store_command and
	// authenticate should be used to finish message construction.
	// "rr" is the identifier of the replica from which the client
	// expects a full reply (if negative, client expects a full reply
	// from all replicas).

	~R_Request();

	static const int big_req_thresh = 0; // Maximum size of not-big requests
	char* store_command(int &max_len);
	// Effects: Returns a pointer to the location within the message
	// where the command should be stored and sets "max_len" to the number of
	// bytes available to store the reply. The caller can copy any command
	// with length less than "max_len" into the returned buffer.

	void set_seqno(Seqno seq);
	// sets seqno
	Seqno& seqno() const { return rep().seqno; };
	bool is_sequenced() const { return rep().seqno != 0; };
	void set_replica(int id) { rep().replica_id = id; };

	void set_type(bool is_ack) { rep().type = (is_ack == true)?0xdeadbeef:0; };

	void finalize(int act_len, bool read_only);

	void authenticate_for_client(int, int, int*, R_Reply_rep*, int);
	void authenticate(int id, int authentication_offset, R_Reply_rep *rr, int iteration);
	// Effects: Terminates the construction of a request message by
	// setting the length of the command to "act_len", and appending an
	// authenticator. read-only should be true iff the request is read-only
	// (i.e., it will not change the service state).

	bool verify_simple(int *authentication_offset, int iteration);
	bool verify(int *authentication_offset, int iteration);

	int patch_authenticators_for_client(char *dest, int offset);
	int copy_authenticators(char *dest);
	// Copies the authenticator to dest
	void replace_authenticators(char *src);

	void swap_seqno(Seqno newseqno, Seqno *oldseqno);
	Seqno get_stored_seqno(int index);
	void set_stored_seqno(int index, Seqno s);
	void roll_stored_seqno();

	int client_id() const;
	int replica_id() const;
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

	bool is_read_only() const;
	// Effects: Returns true iff the request message states that the
	// request is read-only.

	void set_batched();
	// Effects: Sets request to be type batched

	bool is_batched() const;
	// Effects: Returns true if the message is batched.

	void unset_piggybacked();
	void set_piggybacked();
	// Effects: Sets request to be type piggybacked

	bool is_piggybacked() const;
	// Effects: Returns true if the message is piggybacked.

	static bool convert(R_Message *m1, R_Request *&m2);
	// Effects: If "m1" has the right size and tag of a "R_Request",
	// casts "m1" to a "R_Request" pointer, returns the pointer in
	// "m2" and returns true. Otherwise, it returns false.
	Digest& digest() const {
		return rep().od;
	}

	int seqnos_size;

private:
	R_Request_rep &rep() const;
	// Effects: Casts "msg" to a R_Request_rep&

	void comp_digest(Digest& d);
	// Effects: computes the digest of rid, cid, and the command.

	class IteratorData {
	    public:
	    R_Request *oreq; // pointer to the request
	    R_Request *req; // pointer to the extracted request
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

inline R_Request_rep& R_Request::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((R_Request_rep*) msg);
}

inline int R_Request::client_id() const {

	return rep().cid;
}

inline int R_Request::replica_id() const {

	return rep().replica_id;
}

inline Request_id &R_Request::request_id() {
	return rep().rid;
}

inline char *R_Request::command(int &len) {
	len = rep().command_size;
	return contents() + sizeof(R_Request_rep);
}

inline bool R_Request::is_read_only() const {
	// return false;
	return rep().extra & 1;
}

inline char *R_Request::MACs() {
	return contents() + sizeof(R_Request_rep) + rep().command_size + seqnos_size;
}

inline void R_Request::set_batched() {
    rep().extra = rep().extra | R_MESSAGE_BATCH;
}

inline bool R_Request::is_batched() const {
    return rep().extra & R_MESSAGE_BATCH;
}

inline void R_Request::unset_piggybacked() {
    rep().extra = rep().extra && ~R_MESSAGE_PIGGYBACKED;
}

inline void R_Request::set_piggybacked() {
    rep().extra = rep().extra | R_MESSAGE_PIGGYBACKED;
}

inline bool R_Request::is_piggybacked() const {
    return rep().extra & R_MESSAGE_PIGGYBACKED;
}
#endif // _R_Request_h
