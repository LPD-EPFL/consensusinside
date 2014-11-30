#ifndef _DSum_h
#define _DSum_h 1

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>
#include <malloc.h>
#include <string.h>

class Digest;

//extern "C" {
#include "gmp.h"
//}

#include "th_assert.h"

// Force template instantiation
//#include "Array.t"
//#include "Log.t"
//#include "bhash.t"
//#include "buckets.t"
//template class Log<Checkpoint_rec>;


//
// Sums of digests modulo a large integer
//
struct DSum {
  static const int nbits = 256;
  static const int mp_limb_bits = sizeof(mp_limb_t)*8;
  static const int nlimbs = (nbits+mp_limb_bits-1)/mp_limb_bits;
  static const int nbytes = nlimbs*sizeof(mp_limb_t);
  static DSum* M; // Modulus for sums must be at most nbits-1 long.

  mp_limb_t sum[nlimbs];
  //  char dummy[56];

  inline DSum() { bzero(sum, nbytes); }
  // Effects: Creates a new sum object with value 0

  inline DSum(DSum const & other) {
    memcpy(sum, other.sum, nbytes);
  }
  
  inline DSum& operator=(DSum const &other) {
    if (this == &other) return *this;
    memcpy(sum, other.sum, nbytes);
    return *this;
  }

  void add(Digest &d);
  // Effects: adds "d" to this

  void sub(Digest &d);
  // Effects: subtracts "d" from this.
};


#endif
