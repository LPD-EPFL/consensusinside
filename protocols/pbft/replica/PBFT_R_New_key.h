#ifndef _PBFT_R_New_key_h
#define _PBFT_R_New_key_h 1

#include "types.h"
#include "Digest.h"
#include "PBFT_R_Message.h"
#include "PBFT_R_Principal.h"

// 
// PBFT_R_New_key messages have the following format:
//
struct PBFT_R_New_key_rep : public PBFT_R_Message_rep {
  Request_id rid;
  int id; // id of the PBFT_R_replica that generated the message.
  int padding;

  // Followed by keys for all PBFT_R_replicas except "id" in order of
  // increasing identifiers.  Each key has size Nonce_size bytes and
  // is encrypted with the public-key of the corresponding
  // PBFT_R_replica. This is all followed by a signature from principal id
};

class PBFT_R_New_key : public PBFT_R_Message {
  // 
  //  PBFT_R_New_key messages
  //
public:
  PBFT_R_New_key();
  // Effects: Creates a new signed PBFT_R_New_key message and updates "node"
  // accordingly (i.e., updates the in-keys for all principals.) 

  int id() const;
  // Effects: PBFT_R_Fetches the identifier of the PBFT_R_replica from the message.

  bool verify();
  // Effects: Verifies if the message is signed by the principal
  // rep().id. If the message is correct updates the entry for
  // rep().id accordingly (i.e., out-key, tstamp.)

  static bool convert(PBFT_R_Message *m1, PBFT_R_New_key *&m2);
  // Effects: If "m1" has the right size and tag of a "PBFT_R_New_key",
  // casts "m1" to a "PBFT_R_New_key" pointer, returns the pointer in
  // "m2" and returns true. Otherwise, it returns false.
  // If the conversion is successful it trims excess allocation.

private:
  PBFT_R_New_key_rep &rep() const;
  // Effects: Casts "msg" to a PBFT_R_New_key_rep&
};


inline PBFT_R_New_key_rep& PBFT_R_New_key::rep() const { 
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((PBFT_R_New_key_rep*)msg); 
}

inline int PBFT_R_New_key::id() const { return rep().id; }

#endif // _PBFT_R_New_key_h
