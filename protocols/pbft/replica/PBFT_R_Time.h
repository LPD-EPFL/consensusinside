#ifndef _PBFT_R_Time_h
#define _PBFT_R_Time_h 1

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

typedef struct timeval PBFT_R_Time;
static inline PBFT_R_Time currentPBFT_R_Time() {
  PBFT_R_Time t;
  int ret = gettimeofday(&t, 0);
  th_assert(ret == 0, "gettimeofday PBFT_C_failed");
  return t;
}

static inline PBFT_R_Time zeroPBFT_R_Time() {  
  PBFT_R_Time t;
  t.tv_sec = 0;
  t.tv_usec = 0
  return t; 
}

static inline long long diffPBFT_R_Time(PBFT_R_Time t1, PBFT_R_Time t2) {
  // t1-t2 in microseconds.
  return (((unsigned long long)(t1.tv_sec-t2.tv_sec)) << 20) + (t1.tv_usec-t2.tv_usec);
}

static inline bool lessThanPBFT_R_Time(PBFT_R_Time t1, PBFT_R_Time t2) {
  return t1.tv_sec < t2.tv_sec ||  
    (t1.tv_sec == t2.tv_sec &&  t1.tv_usec < t2.tv_usec);
}
#else

typedef long long PBFT_R_Time;

#include "PBFT_R_Cycle_counter.h"

extern unsigned long long clock_mhz;
// Clock frequency in MHz

extern void init_clock_mhz();
// Effects: Initialize "clock_mhz".

static inline PBFT_R_Time currentPBFT_R_Time() { return PBFT_R_rdtsc(); }

static inline PBFT_R_Time zeroPBFT_R_Time() { return 0; }

static inline long long diffPBFT_R_Time(PBFT_R_Time t1, PBFT_R_Time t2) {
  return (t1-t2)/clock_mhz;
}

static inline bool lessThanPBFT_R_Time(PBFT_R_Time t1, PBFT_R_Time t2) {
  return t1 < t2;

}

#endif

#endif // _PBFT_R_Time_h 
