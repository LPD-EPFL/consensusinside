#ifndef _T_Time_h
#define _T_Time_h 1

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

typedef struct timeval T_Time;
static inline T_Time currentT_Time() {
  T_Time t;
  int ret = gettimeofday(&t, 0);
  th_assert(ret == 0, "gettimeofday PBFT_C_failed");
  return t;
}

static inline T_Time zeroT_Time() {  
  T_Time t;
  t.tv_sec = 0;
  t.tv_usec = 0
  return t; 
}

static inline long long diffT_Time(T_Time t1, T_Time t2) {
  // t1-t2 in microseconds.
  return (((unsigned long long)(t1.tv_sec-t2.tv_sec)) << 20) + (t1.tv_usec-t2.tv_usec);
}

static inline bool lessThanT_Time(T_Time t1, T_Time t2) {
  return t1.tv_sec < t2.tv_sec ||  
    (t1.tv_sec == t2.tv_sec &&  t1.tv_usec < t2.tv_usec);
}
#else

typedef long long T_Time;

#include "T_Cycle_counter.h"

extern unsigned long long clock_mhz;
// Clock frequency in MHz

extern void init_clock_mhz();
// Effects: Initialize "clock_mhz".

static inline T_Time currentT_Time() { return T_rdtsc(); }

static inline T_Time zeroT_Time() { return 0; }

static inline long long diffT_Time(T_Time t1, T_Time t2) {
  return (t1-t2)/clock_mhz;
}

static inline bool lessThanT_Time(T_Time t1, T_Time t2) {
  return t1 < t2;

}

#endif

#endif // _T_Time_h 
