#ifndef _Pre_PBFT_C_Prepare_h
#define _Pre_PBFT_C_Prepare_h 1

#include "types.h"
#include "Digest.h"
#include "PBFT_C_Prepare.h"
#include "PBFT_C_Message.h"

class PBFT_C_Principal;
class PBFT_C_Req_queue;
class PBFT_C_Request;

//
// PBFT_C_Pre_prepare messages have the following format:
//
struct PBFT_C_Pre_prepare_rep : public PBFT_C_Message_rep {
  View view;
  Seqno seqno;
  Digest digest;       // digest of request set concatenated with
                       // big reqs and non-deterministic choices
  int rset_size;       // size in bytes of request set
  short n_big_reqs;    // number of big requests
  short non_det_size;  // size in bytes of non-deterministic choices

  // Followed by "rset_size" bytes of the request set, "n_big_reqs"
  // Digest's, "non_det_size" bytes of non-deterministic choices, and
  // a variable length signature in the above order.
};

class PBFT_C_Prepare;

class PBFT_C_Pre_prepare : public PBFT_C_Message {
  //
  // PBFT_C_Pre_prepare messages
  //
public:
  PBFT_C_Pre_prepare() : PBFT_C_Message() {}

  PBFT_C_Pre_prepare(View v, Seqno s, PBFT_C_Req_queue &reqs);
  // Effects: Creates a new signed PBFT_C_Pre_prepare message with view
  // number "v", sequence number "s", the requests in "reqs" (up to a
  // maximum size) and appropriate non-deterministic choices.  It
  // removes the elements of "reqs" that are included in the message
  // from "reqs" and deletes them.

  char* choices(int &len);
  // Effects: Returns a buffer that can be filled with non-deterministic choices

  PBFT_C_Pre_prepare* clone(View v) const;
  // Effects: Creates a new object with the same state as this but view v.

  void re_authenticate(PBFT_C_Principal *p=0);
  // Effects: Recomputes the authenticator in the message using the most
  // recent keys. If "p" is not null, may only update "p"'s
  // entry.

  View view() const;
  // Effects: PBFT_C_Fetches the view number from the message.

  Seqno seqno() const;
  // Effects: PBFT_C_Fetches the sequence number from the message.

  int id() const;
  // Effects: Returns the identifier of the primary for view() (which is
  // the replica that sent the message if the message is correct.)

  bool match(const PBFT_C_Prepare *p) const;
  // Effects: Returns true iff "p" and "this" match.

  Digest& digest() const;
  // Effects: PBFT_C_Fetches the digest from the message.

  class PBFT_C_Requests_iter {
    // An iterator for yielding the PBFT_C_Requests in a PBFT_C_Pre_prepare message.
    // Requires: A PBFT_C_Pre_prepare message cannot be modified while it is
    // being iterated on and all the big requests referred to by "m"
    // must be cached.
  public:
    PBFT_C_Requests_iter(PBFT_C_Pre_prepare* m);
    // Requires: PBFT_C_Pre_prepare is known to be valid
    // Effects: Return an iterator for the requests in "m"

    bool get(PBFT_C_Request& req);
    // Effects: Updates "req" to "point" to the next request in the
    // PBFT_C_Pre_prepare message and returns true. If there are no more
    // requests, it returns false.

  private:
    PBFT_C_Pre_prepare* msg;
    char* next_req;
    int big_req;
  };
  friend  class PBFT_C_Requests_iter;

  // Maximum number of big reqs in pre-prepares.
  static const int big_req_max = sizeof(BR_map)*8;
  int num_big_reqs() const;
  // Effects: Returns the number of big request digests in this

  Digest& big_req_digest(int i);
  // Requires: 0 <= "i" < "num_big_reqs()"
  // Effects: Returns the digest of the i-th big request in this

  static const int NAC = 1;
  static const int NRC = 2;
  bool verify(int mode=0);
  // Effects: If "mode == 0", verifies if the message is authenticated
  // by the replica "id()", if the digest is correct, and if the
  // requests are authentic. If "mode == NAC", it performs all checks
  // except that it does not check if the message is authenticated by
  // the replica "id()". If "mode == NRC", it performs all checks
  // except that it does not verify the authenticity of the requests.

  bool check_digest();
  // Effects: Verifies if the digest is correct.

  static bool convert(PBFT_C_Message* m1, PBFT_C_Pre_prepare*& m2);
  // Effects: If "m1" has the right size and tag, casts "m1" to a
  // "PBFT_C_Pre_prepare" pointer, returns the pointer in "m2" and returns
  // true. Otherwise, it returns false.

private:
  PBFT_C_Pre_prepare_rep& rep() const;
  // Effects: Casts contents to a PBFT_C_Pre_prepare_rep&

  char* requests();
  // Effects: Returns a pointer to the first request contents.

  Digest* big_reqs();
  // Effects: Returns a pointer to the first digest of a big request
  // in this.

  char* non_det_choices();
  // Effects: Returns a pointer to the buffer with non-deterministic
  // choices.
};

inline PBFT_C_Pre_prepare_rep& PBFT_C_Pre_prepare::rep() const {
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((PBFT_C_Pre_prepare_rep*)msg);
}

inline char* PBFT_C_Pre_prepare::requests() {
  char *ret = contents()+sizeof(PBFT_C_Pre_prepare_rep);
  th_assert(ALIGNED(ret), "Improperly aligned pointer");
  return ret;
}

inline  Digest* PBFT_C_Pre_prepare::big_reqs() {
  char *ret = requests()+rep().rset_size;
  th_assert(ALIGNED(ret), "Improperly aligned pointer");
  return (Digest*)ret;
}

inline char* PBFT_C_Pre_prepare::non_det_choices() {
  char *ret = ((char*)big_reqs())+rep().n_big_reqs*sizeof(Digest);
  th_assert(ALIGNED(ret), "Improperly aligned pointer");
  return ret;
}

inline char* PBFT_C_Pre_prepare::choices(int &len) {
  len = rep().non_det_size;
  return non_det_choices();
}

inline View PBFT_C_Pre_prepare::view() const { return rep().view; }

inline Seqno PBFT_C_Pre_prepare::seqno() const { return rep().seqno; }

inline bool PBFT_C_Pre_prepare::match(const PBFT_C_Prepare* p) const {
  th_assert(view() == p->view() && seqno() == p->seqno(), "Invalid argument");
  return digest() == p->digest();
}

inline Digest& PBFT_C_Pre_prepare::digest() const { return rep().digest; }

inline int PBFT_C_Pre_prepare::num_big_reqs() const { return rep().n_big_reqs; }

inline Digest& PBFT_C_Pre_prepare::big_req_digest(int i) {
  th_assert(i >= 0 && i < num_big_reqs(), "Invalid argument");
  return *(big_reqs()+i);
}

#endif // _PBFT_C_Pre_prepare_h
