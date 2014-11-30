// Cumulative timer to measure elapsed time.

#ifndef _PBFT_C_TIMER_H
#define _PBFT_C_TIMER_H 1

#include <sys/time.h>
#include <unistd.h>

inline float diff_time (struct timeval &t0, struct timeval &t1) {
    return (t1.tv_sec-t0.tv_sec)+(t1.tv_usec-t0.tv_usec)/1e6;
    // preserved significant digits by subtracting separately
}

class PBFT_C_Timer {
public:

    PBFT_C_Timer() {reset();}

    // Reset timer to 0.
    inline void reset() {
	running = false;
	accumulated = 0.0;
    }

    // Start timer.
    inline void start() {
	if (!running) {
	    running = true;
            gettimeofday(&t0, 0);
	}
    }

    // Stop timer.
    inline void stop() {
	if (running) {
	    running = false;
	    gettimeofday(&t1, 0);
	    accumulated += diff_time(t0, t1);
	}
    }

    // Return seconds for which timer has run until now
    // since it was created or last reset.
    inline float elapsed() {
	if (running) {
	    gettimeofday(&t1, 0);
	    float runtime = diff_time(t0, t1);
	    return (accumulated+runtime);
	} else {
	    return accumulated;
	}
    }

private:
    struct timeval t0, t1;
    float accumulated;
    bool running;
};


#endif  /* _PBFT_C_TIMER_H */
