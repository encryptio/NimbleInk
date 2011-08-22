#ifndef __ZIPPER_H__
#define __ZIPPER_H__

#include "archive.h"
#include "image.h"
#include "reference.h"

#include <stdbool.h>

// must be at least 3 to preload upcoming images
#define ZIPPER_MAX_NUM_POS 4

struct zipper_pos {
    // NB: no refcount, all allocated statically in struct zipper

    struct cpuimage *cpu;
    struct glimage *gl;

    int updepth;
    char path[MAX_PATH_LENGTH];
    struct {
        bool is;
        int pos;
        struct archive *ar;
    } ar;
};

struct zipper {
    int refcount;

    // TODO: consider circular buffer
    struct zipper_pos pos[ZIPPER_MAX_NUM_POS];
    int pos_at;
    int pos_len; // currently loaded
};

struct zipper * zipper_create(char *path);

bool zipper_next(struct zipper *z);
bool zipper_prev(struct zipper *z);

struct glimage * zipper_current_glimage(struct zipper *z);

// returns true if it wants to be called again
bool zipper_tick_preload(struct zipper *z);

void zipper_incr(void *z);
void zipper_decr(void *z);

static inline void zipper_decr_q(struct zipper *z) {
    ref_queue_decr((void*)(z), zipper_decr);
}

#endif
