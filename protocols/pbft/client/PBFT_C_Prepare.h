#ifndef _PBFT_C_Prepare_h
#define _PBFT_C_Prepare_h 1

#include "types.h"
#include "Digest.h"
#include "PBFT_C_Message.h"
class PBFT_C_Principal;

// 
// PBFT_C_Prepare messages have the following format:
//
struct PBFT_C_Prepare_rep : public PBFT_C_Message_rep {
  View view;       
  Seqno seqno;
  Digest digest;
  int id;         // id of the replica that generated the message.
  int padding;
  // Followed by a variable-sized signature.
};

class PBFT_C_Prepare : public PBFT_C_Message {
  // 
  // PBFT_C_Prepare messages
  //
public:
  PBFT_C_Prepare(View v, Seqno s, Digest &d, PBFT_C_Principal* dst=0);
  // Effects: Creates a new signed PBFT_C_Prepare message with view number
  // "v", sequence number "s" and digest "d". "dst" should be non-null
  // iff prepare is sent to a single replica "dst" as proof of
  // authenticity for a request.

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

  Digest &digest() const;
  // Effects: PBFT_C_Fetches the digest from the message.

  bool is_proof() const;
  // Effects: Returns true iff this was sent as proof of authenticity
  // for a request.

  bool match(const PBFT_C_Prepare *p) const;
  // Effects: Returns true iff "p" and "this" match.

  bool verify();
  // Effects: Verifies if the message is signed by the replica rep().id.

  static bool convert(PBFT_C_Message *m1, PBFT_C_Prepare *&m2);
  // Effects: If "m1" has the right size and tag, casts "m1" to a
  // "PBFT_C_Prepare" pointer, returns the pointer in "m2" and returns
  // true. Otherwise, it returns false. 

private:
  PBFT_C_Prepare_rep &rep() const;
  // Effects: Casts contents to a PBFT_C_Prepare_rep&

};


inline PBFT_C_Prepare_rep &PBFT_C_Prepare::rep() const { 
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((PBFT_C_Prepare_rep*)msg); 
}

inline View PBFT_C_Prepare::view() const { return rep().view; }

inline Seqno PBFT_C_Prepare::seqno() const { return rep().seqno; }

inline int PBFT_C_Prepare::id() const { return rep().id; }

inline Digest& PBFT_C_Prepare::digest() const { return rep().digest; }

inline bool PBFT_C_Prepare::is_proof() const { return rep().extra != 0; }

inline bool PBFT_C_Prepare::match(const PBFT_C_Prepare *p) const { 
  th_assert(view() == p->view() && seqno() == p->seqno(), "Invalid argument");
  return digest() == p->digest();
}


#endif // _PBFT_C_Prepare_h
