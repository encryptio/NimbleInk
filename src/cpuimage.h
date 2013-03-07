#ifndef __CPUIMAGE_H__
#define __CPUIMAGE_H__

#include "path.h"
#include "reference.h"

#include <inttypes.h>
#include <stdbool.h>

// slice order is top down, left to right (x varies fastest)

enum cpuimage_pixel_format {
    CPUIMAGE_GRAY,
    CPUIMAGE_GRAYA,
    CPUIMAGE_RGB,
    CPUIMAGE_BGR,
    CPUIMAGE_RGBA,
    CPUIMAGE_BGRA
};

struct cpuimage {
    int refcount;
    void (*incr)(struct cpuimage *i);
    void (*decr)(struct cpuimage *i);
    void (*decr_q)(struct cpuimage *i);

    uint16_t w;
    uint16_t h;
    
    uint16_t s_w; // slices wide
    uint16_t s_h; // slices tall
    void *slices; // in slice order

    enum cpuimage_pixel_format pf;

    double cpu_time;

    char path[MAX_PATH_LENGTH];
};

struct cpuimage * cpuimage_from_disk(char *path);
struct cpuimage * cpuimage_from_ram(void *ptr, int len);

// internal to the image_* functions
bool cpuimage_setup_cpu_wh(struct cpuimage *i, int w, int h, enum cpuimage_pixel_format pf); // sets w,h and allocates slices

#endif
