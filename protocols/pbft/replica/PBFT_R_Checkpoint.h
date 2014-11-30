#ifndef _PBFT_R_Checkpoint_h
#define _PBFT_R_Checkpoint_h 1

#include "types.h"
#include "Digest.h"
#include "PBFT_R_Message.h"
class PBFT_R_Principal;

// 
// PBFT_R_Checkpoint messages have the following format:
//
struct PBFT_R_Checkpoint_rep : public PBFT_R_Message_rep {
  Seqno seqno;
  Digest digest;
  int id;         // id of the PBFT_R_replica that generated the message.
  int padding;
  // Followed by a variable-sized signature.
};

class PBFT_R_Checkpoint : public PBFT_R_Message {
  // 
  //  PBFT_R_Checkpoint messages
  //
public:
  PBFT_R_Checkpoint(Seqno s, Digest &d, bool stable=false);
  // Effects: Creates a new signed PBFT_R_Checkpoint message with sequence
  // number "s" and digest "d". "stable" should be true iff the checkpoint
  // is known to be stable.

  void re_authenticate(PBFT_R_Principal *p=0, bool stable=false);
  // Effects: Recomputes the authenticator in the message using the
  // most recent keys. "stable" should be true iff the checkpoint is
  // known to be stable.  If "p" is not null, may only update "p"'s
  // entry. XXXX two default args is dangerous try to avoid it

  Seqno seqno() const;
  // Effects: PBFT_R_Fetches the sequence number from the message.

  int id() const;
  // Effects: PBFT_R_Fetches the identifier of the PBFT_R_replica from the message.

  Digest &digest() const;
  // Effects: PBFT_R_Fetches the digest from the message.

  bool stable() const;
  // Effects: Returns true iff the sender of the message believes the
  // checkpoint is stable.

  bool match(const PBFT_R_Checkpoint *c) const;
  // Effects: Returns true iff "c" and "this" have the same digest

  bool verify();
  // Effects: Verifies if the message is signed by the PBFT_R_replica rep().id.

  static bool convert(PBFT_R_Message *m1, PBFT_R_Checkpoint *&m2);
  // Effects: If "m1" has the right size and tag of a "PBFT_R_Checkpoint",
  // casts "m1" to a "PBFT_R_Checkpoint" pointer, returns the pointer in
  // "m2" and returns true. Otherwise, it returns false. Convert also
  // trims any surplus storage from "m1" when the conversion is
  // successfull.
 
private:
  PBFT_R_Checkpoint_rep& rep() const;
  // Effects: Casts "msg" to a PBFT_R_Checkpoint_rep&
};

inline PBFT_R_Checkpoint_rep& PBFT_R_Checkpoint::rep() const { 
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((PBFT_R_Checkpoint_rep*)msg); 
}

inline Seqno PBFT_R_Checkpoint::seqno() const { return rep().seqno; }

inline int PBFT_R_Checkpoint::id() const { return rep().id; }

inline Digest& PBFT_R_Checkpoint::digest() const { return rep().digest; }

inline bool PBFT_R_Checkpoint::stable() const { return rep().extra == 1; }

inline bool PBFT_R_Checkpoint::match(const PBFT_R_Checkpoint *c) const { 
  th_assert(seqno() == c->seqno(), "Invalid argument");
  return digest() == c->digest(); 
}

#endif // _PBFT_R_Checkpoint_h
