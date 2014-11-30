#ifndef _zl_Cycle_countezl_h
#define _zl_Cycle_countezl_h 1

static inline long long zl_rdtsc(void) {
  union {
    struct {
      unsigned int l;  /* least significant word */
      unsigned int h;  /* most significant word */
    } w32;
    unsigned long long w64;
  } v;

  __asm __volatile (".byte 0xf; .byte 0x31     # zl_rdtsc instruction"
                    : "=a" (v.w32.l), "=d" (v.w32.h) :);
  return v.w64;
}


class zl_Cycle_counter {
public:
  zl_Cycle_counter();
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
  // zl_Cycle_counter c; c.start(); c.stop();
  //
  // The purpose is to avoid counting in the measurement overhead.
  static const long long calibration = 37;
};


inline void zl_Cycle_counter::reset() {
  accumulated = 0;
  running = false;
  max_incr = 0;
}


inline zl_Cycle_counter::zl_Cycle_counter() {
  reset();
}


inline void zl_Cycle_counter::start() {
  if (!running) {
    running = true;
    c0 = zl_rdtsc();
  }
}


inline void zl_Cycle_counter::stop() {
  if (running) {
    running = false;
    c1 = zl_rdtsc();
    long long incr = c1-c0-calibration;
    if (incr > max_incr) max_incr = incr;
    accumulated += incr;
  }
}


inline long long zl_Cycle_counter::elapsed() {
  if (running) {
    c1 = zl_rdtsc();
    return (accumulated+c1-c0-calibration);
  } else {
    return accumulated;
  }
}

inline long long zl_Cycle_counter::max_increment() {
  return max_incr;
}

#endif // _zl_Cycle_countezl_h
