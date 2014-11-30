#ifndef _PBFT_C_Pre_prepare_info_h
#define _PBFT_C_Pre_prepare_info_h 1

#include "types.h"
#include "PBFT_C_Pre_prepare.h"

class PBFT_C_Pre_prepare_info {
  //
  // Holds information about a pre-prepare and matching big requests.
  //
public:
  PBFT_C_Pre_prepare_info();
  // Effects: Creates an empty object.

  ~PBFT_C_Pre_prepare_info();
  // Effects: Discard this and any pre-prepare message it may contain.

  void add(PBFT_C_Pre_prepare* p);
  // Effects: Adds "p" to this.

  void add_complete(PBFT_C_Pre_prepare* p);
  // Effects: Adds "p" to this and records that all the big reqs it
  // refers to are cached.

  void add(Digest &rd, int i);
  // Effects: If there is a pre-prepare in this and its i-th reference
  // to a big request is for the request with digest rd record that
  // this digest is cached.

  PBFT_C_Pre_prepare* pre_prepare() const;
  // Effects: If there is a pre-prepare message in this returns
  // it. Otherwise, returns 0.

  BR_map missing_reqs() const;
  // Effects: Returns a bit map with the indices of missing requests.

  bool is_complete() const;
  // Effects: Returns true iff this contains a pre-prepare and all the
  // big requests it references are cached.

  void clear();
  // Effects: Makes this empty and deletes any pre-prepare in it.

  void zero();
  // Effects: Makes this empty without deleting any contained
  // pre-prepare.

  class BRS_iter {
    // An iterator for yielding the requests in ppi that are missing
    // in mrmap.
  public:
    BRS_iter(PBFT_C_Pre_prepare_info const *p, BR_map m);
    // Effects: Return an iterator for the missing requests in mrmap

    bool get(PBFT_C_Request*& r);
    // Effects: Sets "r" to the next request in this that is missing
    // in mrmap, and returns true. Unless there are no more missing
    // requests in this, in which case it returns false.

  private:
    PBFT_C_Pre_prepare_info const * ppi;
    BR_map mrmap;
    int next;
  };
  friend  class BRS_iter;

  bool encode(FILE* o);
  bool decode(FILE* i);
  // Effects: Encodes and decodes object state from stream. Return
  // true if successful and false otherwise.

private:
  PBFT_C_Pre_prepare* pp;
  short mreqs;    // Number of missing requests
  BR_map mrmap;   // PBFT_C_Bitmap with bit reset for every missing request
};


inline PBFT_C_Pre_prepare_info::PBFT_C_Pre_prepare_info() {
  pp = 0;
}


inline void PBFT_C_Pre_prepare_info::zero() {
  pp = 0;
}


inline void PBFT_C_Pre_prepare_info::add_complete(PBFT_C_Pre_prepare* p) {
  th_assert(pp == 0, "Invalid state");
  pp = p;
  mreqs = 0;
  mrmap = ~0;
}


inline PBFT_C_Pre_prepare* PBFT_C_Pre_prepare_info::pre_prepare() const {
  return pp;
}


inline BR_map PBFT_C_Pre_prepare_info::missing_reqs() const {
  return mrmap;
}


inline void PBFT_C_Pre_prepare_info::clear() {
  delete pp;
  pp = 0;
}


inline bool PBFT_C_Pre_prepare_info::is_complete() const {
  return pp != 0 && mreqs == 0;
}

#endif //_PBFT_C_Pre_prepare_info_h
