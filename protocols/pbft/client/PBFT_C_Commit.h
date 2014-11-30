#ifndef _PBFT_C_Commit_h
#define _PBFT_C_Commit_h 1

#include "types.h"
#include "PBFT_C_Message.h"
class PBFT_C_Principal;

// 
// PBFT_C_Commit messages have the following format.
//
struct PBFT_C_Commit_rep : public PBFT_C_Message_rep {
  View view;       
  Seqno seqno;
  int id;         // id of the replica that generated the message.
  int padding;
  // Followed by a variable-sized signature.
};

class PBFT_C_Commit : public PBFT_C_Message {
  // 
  // PBFT_C_Commit messages
  //
public:
  PBFT_C_Commit(View v, Seqno s);
  // Effects: Creates a new PBFT_C_Commit message with view number "v"
  // and sequence number "s".

  PBFT_C_Commit(PBFT_C_Commit_rep *contents);
  // Requires: "contents" contains a valid PBFT_C_Commit_rep. If
  // contents may not be a valid PBFT_C_Commit_rep use the static
  // method convert.
  // Effects: Creates a PBFT_C_Commit message from "contents". No copy
  // is made of "contents" and the storage associated with "contents"
  // is not deallocated if the message is later deleted.

  void re_authenticate(PBFT_C_Principal *p=0);
  // Effects: Recomputes the authenticator in the message using the
  // most recent keys. If "p" is not null, may only update "p"'s
  // entry.

  View view() const;
  // Effects: PBFT_C_Fetches the view number from the message.

  Seqno seqno() const;
  // Effects: PBFT_C_Fetches the sequence number from the message.

  int id() const;
  // Effects: PBFT_C_Fetches the identifier of the replica from the message.

  bool match(const PBFT_C_Commit *c) const;
  // Effects: Returns true iff this and c match.

  bool verify();
  // Effects: Verifies if the message is signed by the replica rep().id.

  static bool convert(PBFT_C_Message *m1, PBFT_C_Commit *&m2);
  // Effects: If "m1" has the right size and tag of a "PBFT_C_Commit",
  // casts "m1" to a "PBFT_C_Commit" pointer, returns the pointer in
  // "m2" and returns true. Otherwise, it returns false. 

  static bool convert(char *m1, unsigned max_len, PBFT_C_Commit &m2);
  // Requires: convert can safely read up to "max_len" bytes starting
  // at "m1".
  // Effects: If "m1" has the right size and tag of a
  // "PBFT_C_Commit_rep" assigns the corresponding PBFT_C_Commit to m2 and
  // returns true.  Otherwise, it returns false.  No copy is made of
  // m1 and the storage associated with "contents" is not deallocated
  // if "m2" is later deleted.

 
private:
  PBFT_C_Commit_rep &rep() const;
  // Effects: Casts "msg" to a PBFT_C_Commit_rep&
};


inline PBFT_C_Commit_rep& PBFT_C_Commit::rep() const { 
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((PBFT_C_Commit_rep*)msg); 
}

inline View PBFT_C_Commit::view() const { return rep().view; }

inline Seqno PBFT_C_Commit::seqno() const { return rep().seqno; }

inline int PBFT_C_Commit::id() const { return rep().id; }

inline bool PBFT_C_Commit::match(const PBFT_C_Commit *c) const {
  th_assert(view() == c->view() && seqno() == c->seqno(), "Invalid argument");
  return true; 
}

#endif // _PBFT_C_Commit_h
