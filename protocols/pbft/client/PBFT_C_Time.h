#ifndef _PBFT_C_Time_h
#define _PBFT_C_Time_h 1

/*
 * Definitions of various types.
 */
#include <limits.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef USE_GETTIMEOFDAY

typedef struct timeval PBFT_C_Time;
static inline PBFT_C_Time currentPBFT_C_Time() {
  PBFT_C_Time t;
  int ret = gettimeofday(&t, 0);
  th_assert(ret == 0, "gettimeofday PBFT_C_failed");
  return t;
}

static inline PBFT_C_Time zeroPBFT_C_Time() {
  PBFT_C_Time t;
  t.tv_sec = 0;
  t.tv_usec = 0
  return t;
}

static inline long long diffPBFT_C_Time(PBFT_C_Time t1, PBFT_C_Time t2) {
  // t1-t2 in microseconds.
  return (((unsigned long long)(t1.tv_sec-t2.tv_sec)) << 20) + (t1.tv_usec-t2.tv_usec);
}

static inline bool lessThanPBFT_C_Time(PBFT_C_Time t1, PBFT_C_Time t2) {
  return t1.tv_sec < t2.tv_sec ||
    (t1.tv_sec == t2.tv_sec &&  t1.tv_usec < t2.tv_usec);
}
#else

typedef long long PBFT_C_Time;

#include "PBFT_C_Cycle_counter.h"

extern long long PBFT_C_clock_mhz;
// Clock frequency in MHz

extern void PBFT_C_init_PBFT_C_clock_mhz();
// Effects: Initialize "PBFT_C_clock_mhz".

static inline PBFT_C_Time currentPBFT_C_Time() { return PBFT_C_rdtsc(); }

static inline PBFT_C_Time zeroPBFT_C_Time() { return 0; }

static inline long long diffPBFT_C_Time(PBFT_C_Time t1, PBFT_C_Time t2) {
  return (t1-t2)/PBFT_C_clock_mhz;
}

static inline bool lessThanPBFT_C_Time(PBFT_C_Time t1, PBFT_C_Time t2) {
  return t1 < t2;

}

#endif

#endif // _PBFT_C_Time_h
