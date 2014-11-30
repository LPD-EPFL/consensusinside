#ifndef _PBFT_R_Meta_data_h
#define _PBFT_R_Meta_data_h 1

#include "types.h"
#include "Digest.h"
#include "PBFT_R_Message.h"


// 
// PBFT_R_Meta_data messages contain information about a partition and its
// subpartitions. They have the following format:
//
struct Part_info {
  int i;     // index of sub-partition within its level
  Digest d;  // digest of sub-partition 
};

struct PBFT_R_Meta_data_rep : public PBFT_R_Message_rep {
  Request_id rid;  // timestamp of fetch request
  Seqno lu;        // last seqno for which information in this is up-to-date
  Seqno lm;        // seqno of last checkpoint that modified partition
  int l;           // level of partition in hierarchy
  int i;           // index of partition within level
  Digest d;        // partition's digest
  int id;          // id of sender
  int np;          // number of sub-partitions included in message (i.e.,
                   // sub-partitions modified by a checkpoint with seqno 
                   // greater than the lu on fetch)
  // Part_info parts[num_partitions]; // array of subpartition information
};

class PBFT_R_Meta_data : public PBFT_R_Message {
  // 
  //  PBFT_R_Meta_data messages
  //
public:
  PBFT_R_Meta_data(Request_id r, int l, int i, Seqno lu, Seqno lm, Digest& d);
  // Effects: Creates a new un-authenticated PBFT_R_Meta_data message with no
  // subpartition information.

  void add_sub_part(int index, Digest& digest);
  // Effects: Adds information about the subpartition "index" to this.
  
  Request_id request_id() const;
  // Effects: PBFT_R_Fetches the request identifier from the message.

  Seqno last_uptodate() const;
  // Effects: PBFT_R_Fetches the last seqno at which partition is up-to-date at sending PBFT_R_replica.

  Seqno last_mod() const;
  // Effects: PBFT_R_Fetches seqno of last checkpoint that modified partition.

  int level() const;
  // Effects: Returns the level of the partition  

  int index() const;
  // Effects: Returns the index of the partition within its level

  int id() const;
  // Effects: PBFT_R_Fetches the identifier of the PBFT_R_replica from the message.

  Digest& digest();
  // Effects: Returns the digest of the partition.

  int num_sparts() const;
  // Effects: Returns the number of subpartitions in this.
  
  class Sub_parts_iter {
    // An iterator for yielding all the sub_partitions of this partition
    // in order.
  public:
    Sub_parts_iter(PBFT_R_Meta_data* m);
    // Requires: PBFT_R_Meta_data is known to be valid
    // Effects: Return an iterator for the sub-partitions of the partition
    // in "m".
    
    bool get(int& index, Digest& d);
    // Effects: Modifies "d" to contain the digest of the next
    // subpartition and returns true. If there are no more
    // subpartitions, it returns false. It returns null digests for
    // subpartitions that were not modified since "f->seqno()", where
    // "f" is the fetch message that triggered this reply.
    
  private:
    PBFT_R_Meta_data* msg; 
    int cuPBFT_R_mod;
    int max_mod;
    int index;
    int max_index;
  };
  friend class Sub_parts_iter;
  
  bool verify();
  // Effects: Verifies if the message is correct
  
  static bool convert(PBFT_R_Message *m1, PBFT_R_Meta_data *&m2);
  // Effects: If "m1" has the right size and tag of a "PBFT_R_Meta_data",
  // casts "m1" to a "PBFT_R_Meta_data" pointer, returns the pointer in
  // "m2" and returns true. Otherwise, it returns false. Convert also
  // trims any surplus storage from "m1" when the conversion is
  // successfull.
  
private:
  PBFT_R_Meta_data_rep &rep() const;
  // Effects: Casts "msg" to a PBFT_R_Meta_data_rep&

  Part_info *parts();
};


inline PBFT_R_Meta_data_rep &PBFT_R_Meta_data::rep() const { 
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((PBFT_R_Meta_data_rep*)msg); 
}
 
inline Part_info *PBFT_R_Meta_data::parts() {
  return (Part_info *)(contents()+sizeof(PBFT_R_Meta_data_rep));
}

inline Request_id PBFT_R_Meta_data::request_id() const { return rep().rid; }

inline Seqno PBFT_R_Meta_data::last_uptodate() const { return rep().lu; }

inline Seqno PBFT_R_Meta_data::last_mod() const { return rep().lm; }

inline int PBFT_R_Meta_data::level() const { return rep().l; }

inline int PBFT_R_Meta_data::index() const { return rep().i; }

inline int PBFT_R_Meta_data::id() const { return rep().id; }

inline Digest& PBFT_R_Meta_data::digest() { return rep().d; }

inline int PBFT_R_Meta_data::num_sparts() const { return rep().np; }

#endif // _PBFT_R_Meta_data_h
