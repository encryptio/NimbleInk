#ifndef __TIMING_H__
#define __TIMING_H__

#include <sys/time.h>

static inline double current_timestamp(void) {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double) tp.tv_sec) + ((double) tp.tv_usec) / 1000000.0;
}

#endif
