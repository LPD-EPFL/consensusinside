#ifndef _PBFT_C_Query_stable_h
#define _PBFT_C_Query_stable_h 1

#include "types.h"
#include "PBFT_C_Message.h"
#include "PBFT_C_Principal.h"

// 
// PBFT_C_Query_stable messages have the following format:
//
struct PBFT_C_Query_stable_rep : public PBFT_C_Message_rep {
  int id;         // id of the replica that generated the message.
  int nonce;
  // Followed by a variable-sized signature.
};

class PBFT_C_Query_stable : public PBFT_C_Message {
  // 
  //  PBFT_C_Query_stable messages
  //
public:
  PBFT_C_Query_stable();
  // Effects: Creates a new authenticated PBFT_C_Query_stable message.

  void re_authenticate(PBFT_C_Principal *p=0);
  // Effects: Recomputes the authenticator in the message using the
  // most recent keys. 

  int id() const;
  // Effects: PBFT_C_Fetches the identifier of the replica from the message.

  int nonce() const;
  // Effects: PBFT_C_Fetches the nonce in the message.

  bool verify();
  // Effects: Verifies if the message is signed by the replica rep().id.

  static bool convert(PBFT_C_Message *m1, PBFT_C_Query_stable *&m2);
  // Effects: If "m1" has the right size and tag of a "PBFT_C_Query_stable",
  // casts "m1" to a "PBFT_C_Query_stable" pointer, returns the pointer in
  // "m2" and returns true. Otherwise, it returns false. Convert also
  // trims any surplus storage from "m1" when the conversion is
  // successfull.
 
private:
  PBFT_C_Query_stable_rep& rep() const;
  // Effects: Casts "msg" to a PBFT_C_Query_stable_rep&
};

inline PBFT_C_Query_stable_rep& PBFT_C_Query_stable::rep() const { 
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((PBFT_C_Query_stable_rep*)msg); 
}

inline int PBFT_C_Query_stable::id() const { return rep().id; }

inline int PBFT_C_Query_stable::nonce() const { return rep().nonce; }

#endif // _PBFT_C_Query_stable_h
