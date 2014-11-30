#ifndef _PBFT_R_Meta_data_d_h
#define _PBFT_R_Meta_data_d_h 1

#include "parameters.h"
#include "types.h"
#include "Digest.h"
#include "PBFT_R_Message.h"
class PBFT_R_Principal;

// 
// PBFT_R_Meta_data_d messages contain the digests of a partition for all the
// checkpoints in the state of the sending PBFT_R_replica. They have the
// following format:
//
struct PBFT_R_Meta_data_d_rep : public PBFT_R_Message_rep {
  Request_id rid;  // timestamp of fetch request
  Seqno ls;        // sequence number of last checkpoint known to be stable at sender
  int l;           // level of meta-data information in hierarchy
  int i;           // index of partition within level

  // Digests for partition for each checkpoint held by the sender in
  // order of increasing sequence number. A null digest means the
  // sender does not have the corresponding checkpoint state.
  Digest digests[max_out/checkpoint_interval+1]; 
  int n_digests;   // number of digests in digests

  int id;          // id of sender
  // Followed by a MAC.
};

class PBFT_R_Meta_data_d : public PBFT_R_Message {
  // 
  //  PBFT_R_Meta_data_d messages
  //
public:
  PBFT_R_Meta_data_d(Request_id r, int l, int i, Seqno ls);
  // Effects: Creates a new un-authenticated PBFT_R_Meta_data_d message with no
  // partition digests.

  void add_digest(Seqno n, Digest& digest);
  // Requires: "n%checkpoint_interval = 0", and "last_stable() <= n <=
  // last_stable()+max_out".
  // Effects: Adds the digest of the partition for sequence number "n" to this.

  void authenticate(PBFT_R_Principal *p);
  // Effects: Computes a MAC for the message with the key shared with
  // "p" using the most recent keys.
  
  Request_id request_id() const;
  // Effects: PBFT_R_Fetches the request identifier from the message.

  Seqno last_stable() const;
  // Effects: PBFT_R_Fetches the sequence number of last stable checkpoint in message.

  Seqno last_checkpoint() const;
  // Effects: PBFT_R_Fetches the sequence number of last stable checkpoint in message.

  int num_digests() const;
  // Effects: Returns the number of digests in the message.

  int level() const;
  // Effects: Returns the level of the partition  

  int index() const;
  // Effects: Returns the index of the partition within its level

  int id() const;
  // Effects: PBFT_R_Fetches the identifier of the PBFT_R_replica from the message.

  bool digest(Seqno n, Digest& d);
  // Effects: If there is a digest for this partition at sequence
  // number "n" sets "d" to its value and returns true. Otherwise,
  // returns false.
  
  bool verify();
  // Effects: Verifies if the message is correct and authenticated by
  // PBFT_R_replica rep().id.
  
  static bool convert(PBFT_R_Message *m1, PBFT_R_Meta_data_d *&m2);
  // Effects: If "m1" has the right size and tag of a "PBFT_R_Meta_data_d",
  // casts "m1" to a "PBFT_R_Meta_data_d" pointer, returns the pointer in
  // "m2" and returns true. Otherwise, it returns false. Convert also
  // trims any surplus storage from "m1" when the conversion is
  // successfull.
  
private:
  PBFT_R_Meta_data_d_rep &rep() const;
  // Effects: Casts "msg" to a PBFT_R_Meta_data_d_rep&
};

inline PBFT_R_Meta_data_d_rep &PBFT_R_Meta_data_d::rep() const { 
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((PBFT_R_Meta_data_d_rep*)msg); 
}
 
inline Request_id PBFT_R_Meta_data_d::request_id() const { return rep().rid; }

inline Seqno PBFT_R_Meta_data_d::last_stable() const { return rep().ls; }

inline Seqno PBFT_R_Meta_data_d::last_checkpoint() const { 
  return rep().ls+(rep().n_digests-1)*checkpoint_interval; 
}

inline int PBFT_R_Meta_data_d::num_digests() const { return rep().n_digests; }

inline int PBFT_R_Meta_data_d::level() const { return rep().l; }

inline int PBFT_R_Meta_data_d::index() const { return rep().i; }

inline int PBFT_R_Meta_data_d::id() const { return rep().id; }


#endif // _PBFT_R_Meta_data_d_h
