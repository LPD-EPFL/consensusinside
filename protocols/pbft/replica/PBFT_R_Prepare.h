#ifndef _PBFT_R_Prepare_h
#define _PBFT_R_Prepare_h 1

#include "types.h"
#include "Digest.h"
#include "PBFT_R_Message.h"
class PBFT_R_Principal;

// 
// PBFT_R_Prepare messages have the following format:
//
struct PBFT_R_Prepare_rep : public PBFT_R_Message_rep {
  View view;       
  Seqno seqno;
  Digest digest;
  int id;         // id of the PBFT_R_replica that generated the message.
  int padding;
  // Followed by a variable-sized signature.
};

class PBFT_R_Prepare : public PBFT_R_Message {
  // 
  // PBFT_R_Prepare messages
  //
public:
  PBFT_R_Prepare(View v, Seqno s, Digest &d, PBFT_R_Principal* dst=0);
  // Effects: Creates a new signed PBFT_R_Prepare message with view number
  // "v", sequence number "s" and digest "d". "dst" should be non-null
  // iff prepare is sent to a single PBFT_R_replica "dst" as proof of
  // authenticity for a request.

  void re_authenticate(PBFT_R_Principal *p=0);
  // Effects: Recomputes the authenticator in the message using the
  // most recent keys. If "p" is not null, may only update "p"'s
  // entry.


  View view() const;
  // Effects: PBFT_R_Fetches the view number from the message.

  Seqno seqno() const;
  // Effects: PBFT_R_Fetches the sequence number from the message.

  int id() const;
  // Effects: PBFT_R_Fetches the identifier of the PBFT_R_replica from the message.

  Digest &digest() const;
  // Effects: PBFT_R_Fetches the digest from the message.

  bool is_proof() const;
  // Effects: Returns true iff this was sent as proof of authenticity
  // for a request.

  bool match(const PBFT_R_Prepare *p) const;
  // Effects: Returns true iff "p" and "this" match.

  bool verify();
  // Effects: Verifies if the message is signed by the PBFT_R_replica rep().id.

  static bool convert(PBFT_R_Message *m1, PBFT_R_Prepare *&m2);
  // Effects: If "m1" has the right size and tag, casts "m1" to a
  // "PBFT_R_Prepare" pointer, returns the pointer in "m2" and returns
  // true. Otherwise, it returns false. 

private:
  PBFT_R_Prepare_rep &rep() const;
  // Effects: Casts contents to a PBFT_R_Prepare_rep&

};


inline PBFT_R_Prepare_rep &PBFT_R_Prepare::rep() const { 
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((PBFT_R_Prepare_rep*)msg); 
}

inline View PBFT_R_Prepare::view() const { return rep().view; }

inline Seqno PBFT_R_Prepare::seqno() const { return rep().seqno; }

inline int PBFT_R_Prepare::id() const { return rep().id; }

inline Digest& PBFT_R_Prepare::digest() const { return rep().digest; }

inline bool PBFT_R_Prepare::is_proof() const { return rep().extra != 0; }

inline bool PBFT_R_Prepare::match(const PBFT_R_Prepare *p) const { 
  th_assert(view() == p->view() && seqno() == p->seqno(), "Invalid argument");
  return digest() == p->digest();
}


#endif // _PBFT_R_Prepare_h
