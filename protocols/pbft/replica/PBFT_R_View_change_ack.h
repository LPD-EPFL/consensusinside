#ifndef _PBFT_R_View_change_ack_h
#define _PBFT_R_View_change_ack_h 1

#include "types.h"
#include "Digest.h"
#include "PBFT_R_Message.h"
class PBFT_R_Principal;

// 
// PBFT_R_View_change_ack messages have the following format:
//
struct PBFT_R_View_change_ack_rep : public PBFT_R_Message_rep {
  View v;
  int id;
  int vcid;
  Digest vcd;
  // Followed by a MAC for the intended recipient
};

class PBFT_R_View_change_ack : public PBFT_R_Message {
  // 
  // PBFT_R_View_change_ack messages
  //
public:
  PBFT_R_View_change_ack(View v, int id, int vcid, Digest const &vcd);
  // Effects: Creates a new authenticated PBFT_R_View_change_ack message for
  // PBFT_R_replica "id" stating that PBFT_R_replica "vcid" sent out a view-change
  // message for view "v" with digest "vcd". The MAC is for the primary
  // of "v".

  void re_authenticate(PBFT_R_Principal *p=0);
  // Effects: Recomputes the MAC in the message using the
  // most recent keys. If "p" is not null, computes a MAC for "p"
  // rather than for the primary of "view()".

  View view() const;
  // Effects: PBFT_R_Fetches the view number from the message.

  int id() const;
  // Effects: PBFT_R_Fetches the identifier of the PBFT_R_replica from the message.
  
  int vc_id() const;
  // Effects: PBFT_R_Fetches the identifier of the PBFT_R_replica whose view-change
  // message is being acked.
 
  Digest& vc_digest() const;
  // Effects: PBFT_R_Fetches the digest of the view-change message that is
  // being acked.

  bool match(const PBFT_R_View_change_ack* p) const;
  // Effects: Returns true iff "p" and "this" match.

  bool verify();
  // Effects: Verifies if the message is signed by the PBFT_R_replica rep().id.

  static bool convert(PBFT_R_Message *m1, PBFT_R_View_change_ack *&m2);
  // Effects: If "m1" has the right size and tag, casts "m1" to a
  // "PBFT_R_View_change_ack" pointer, returns the pointer in "m2" and returns
  // true. Otherwise, it returns false. 

private:
  PBFT_R_View_change_ack_rep& rep() const;
  // Effects: Casts contents to a PBFT_R_View_change_ack_rep&

};


inline PBFT_R_View_change_ack_rep& PBFT_R_View_change_ack::rep() const { 
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((PBFT_R_View_change_ack_rep*)msg); 
}

inline View PBFT_R_View_change_ack::view() const { return rep().v; }

inline int PBFT_R_View_change_ack::id() const { return rep().id; }

inline int PBFT_R_View_change_ack::vc_id() const { return rep().vcid; }

inline Digest& PBFT_R_View_change_ack::vc_digest() const { return rep().vcd; }

inline bool PBFT_R_View_change_ack::match(const PBFT_R_View_change_ack *p) const { 
  th_assert(view() == p->view(), "Invalid argument");
  return vc_id() == p->vc_id() && vc_digest() == p->vc_digest();
}

#endif // _PBFT_R_View_change_ack_h
