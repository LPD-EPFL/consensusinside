#ifndef _R_Checkpoint_h
#define _R_Checkpoint_h 1

#include "R_Message.h"
#include "types.h"
#include "R_Reply.h"

class Digest;
class R_Principal;

//
// R_Checkpoint messages have the following format.
//
struct R_Checkpoint_rep: public R_Message_rep {
	int id; // id of the replica which generated the message
	Seqno seqid; // sequence number of the last message in the request history
	Digest digest; // Digest of the history
	// Followed a command which is "command_size" bytes long and an
	// authenticator.
	// Digest resp_digest;
};

class R_Checkpoint: public R_Message {
	//
	// R_Checkpoint messages:
	//
	// Requires: Requests that may have been allocated by library users
	// through the libbyz.h interface can not be trimmed (this could free
	// memory the user expects to be able to use.)
	//
public:
	R_Checkpoint();
	// Effects: Creates a new R_Checkpoint message with an empty
	// command and no authentication.

	virtual ~R_Checkpoint();

	static const int big_req_thresh = 0; // Maximum size of not-big requests

	Digest &digest() const { return rep().digest; }
	// Effects: Fetches the digest from the message.

	void set_digest(Digest digest);
	// Effects: sets the digest for the message.

	int id() const { return rep().id; }
	// Effects: Fetches the identifier of the PBFT_R_replica from the message.

	bool stable() const;
	// Effects: Returns true iff the sender of the message believes the
	// checkpoint is stable.

	void re_authenticate(R_Principal *p, bool stable);
	// Effects: Recomputes the authenticator in the message using the
	// most recent keys. "stable" should be true iff the checkpoint is
	// known to be stable.  If "p" is not null, may only update "p"'s
	// entry. XXXX two default args is dangerous try to avoid it

	bool verify();

	bool match(const R_Checkpoint *c) const;
	// Effects: Returns true iff "c" and "this" have the same digest

	Seqno& get_seqno() const;
	// Effects: Fetches the request identifier of the last message in checkpoint

	void set_seqno(Seqno chkid);
	// Effects: Sets the identifier of the last message in checkpoint

	static bool convert(R_Message *m1, R_Checkpoint *&m2);
	// Effects: If "m1" has the right size and tag of a "R_Checkpoint",
	// casts "m1" to a "R_Checkpoint" pointer, returns the pointer in
	// "m2" and returns true. Otherwise, it returns false.
	//virtual Digest& digest() const {
	//	return rep().od;
	//}

private:
	R_Checkpoint_rep &rep() const;
	// Effects: Casts "msg" to a R_Checkpoint_rep&
};

inline R_Checkpoint_rep& R_Checkpoint::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((R_Checkpoint_rep*) msg);
}

inline void R_Checkpoint::set_digest(Digest digest) {
	rep().digest = digest;
}

inline void R_Checkpoint::set_seqno(Seqno chkid) {
	rep().seqid = chkid;
}

inline Seqno &R_Checkpoint::get_seqno() const {
	return rep().seqid;
}

inline bool R_Checkpoint::match(const R_Checkpoint *c) const
{
  th_assert(get_seqno() == c->get_seqno(), "Invalid argument");
  return digest() == c->digest();
}

#endif // _R_Checkpoint_h
