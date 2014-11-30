#ifndef _T_Cycle_counteT_h
#define _T_Cycle_counteT_h 1

static inline long long T_rdtsc(void) {
  union {
    struct {
      unsigned int l;  /* least significant word */
      unsigned int h;  /* most significant word */
    } w32;
    unsigned long long w64;
  } v;

  __asm __volatile (".byte 0xf; .byte 0x31     # T_rdtsc instruction"
                    : "=a" (v.w32.l), "=d" (v.w32.h) :);
  return v.w64;
}


class T_Cycle_counter {
public:
  T_Cycle_counter();
  // Effects: Create stopped counter with 0 cycles.

  void reset();
  // Effects: Reset counter to 0 and stop it.

  void start();
  // Effects: Start counter.

  void stop();
  // Effects: Stop counter and accumulate cycles since last started.

  long long elapsed();
  // Effects: Return cycles for which counter has run until now since
  // it was created or last reset.

  long long max_increment();
  // Effects: Return maximum number of cycles added to "accummulated" by "stop()"

private:
  long long c0, c1;
  long long accumulated;
  long long max_incr;
  bool running;

  // This variable should be set to the "average" value of c.elapsed()
  // after:
  // T_Cycle_counter c; c.start(); c.stop();
  //
  // The purpose is to avoid counting in the measurement overhead.
  static const long long calibration = 37;
};


inline void T_Cycle_counter::reset() {
  accumulated = 0;
  running = false;
  max_incr = 0;
}


inline T_Cycle_counter::T_Cycle_counter() {
  reset();
}


inline void T_Cycle_counter::start() {
  if (!running) {
    running = true;
    c0 = T_rdtsc();
  }
}


inline void T_Cycle_counter::stop() {
  if (running) {
    running = false;
    c1 = T_rdtsc();
    long long incr = c1-c0-calibration;
    if (incr > max_incr) max_incr = incr;
    accumulated += incr;
  }
}


inline long long T_Cycle_counter::elapsed() {
  if (running) {
    c1 = T_rdtsc();
    return (accumulated+c1-c0-calibration);
  } else {
    return accumulated;
  }
}

inline long long T_Cycle_counter::max_increment() {
  return max_incr;
}

#endif // _T_Cycle_counteT_h
