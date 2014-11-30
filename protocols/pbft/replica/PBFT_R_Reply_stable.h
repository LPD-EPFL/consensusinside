#ifndef _PBFT_R_Reply_stable_h
#define _PBFT_R_Reply_stable_h 1

#include "types.h"
#include "Digest.h"
#include "PBFT_R_Message.h"
class PBFT_R_Principal;

// 
// PBFT_R_Reply_stable messages have the following format:
//
struct PBFT_R_Reply_stable_rep : public PBFT_R_Message_rep {
  Seqno lc;  // last checkpoint at sending PBFT_R_replica
  Seqno lp;  // last prepared request at sending PBFT_R_replica
  int id;    // id of sending PBFT_R_replica
  int nonce; // nonce in query-stable
  // Followed by a MAC
};

class PBFT_R_Reply_stable : public PBFT_R_Message {
  // 
  //  PBFT_R_Reply_stable messages
  //
public:
  PBFT_R_Reply_stable(Seqno lc, Seqno lp, int n, PBFT_R_Principal* p);
  // Effects: Creates a new authenticated PBFT_R_Reply_stable message with
  // last checkpoint "lc", last prepared request "lp", for a
  // query-stable with nonce "n" from principal "p".

  void re_authenticate(PBFT_R_Principal* p);
  // Effects: Recomputes the MAC in the message using the most recent
  // keys.

  Seqno last_checkpoint() const;
  // Effects: PBFT_R_Fetches the sequence number of the last checkpoint from
  // the message.

  Seqno last_prepared() const;
  // Effects: PBFT_R_Fetches the sequence number of the last prepared request
  // from the message.

  int id() const;
  // Effects: PBFT_R_Fetches the identifier of the sender from the message.

  int nonce() const;
  // Effects: PBFT_R_Fetches the nonce of the query from the message.

  bool verify();
  // Effects: Verifies if the message is authenticated by "id()".

  static bool convert(PBFT_R_Message *m1, PBFT_R_Reply_stable *&m2);
  // Effects: If "m1" has the right size and tag of a "PBFT_R_Reply_stable",
  // casts "m1" to a "PBFT_R_Reply_stable" pointer, returns the pointer in
  // "m2" and returns true. Otherwise, it returns false. Convert also
  // trims any surplus storage from "m1" when the conversion is
  // successfull.
 
private:
  PBFT_R_Reply_stable_rep& rep() const;
  // Effects: Casts "msg" to a PBFT_R_Reply_stable_rep&
};

inline PBFT_R_Reply_stable_rep& PBFT_R_Reply_stable::rep() const { 
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((PBFT_R_Reply_stable_rep*)msg); 
}

inline Seqno PBFT_R_Reply_stable::last_checkpoint() const { return rep().lc; }

inline Seqno PBFT_R_Reply_stable::last_prepared() const { return rep().lp; }

inline int PBFT_R_Reply_stable::id() const { return rep().id; }

inline int PBFT_R_Reply_stable::nonce() const { return rep().nonce; }

#endif // _PBFT_R_Reply_stable_h
