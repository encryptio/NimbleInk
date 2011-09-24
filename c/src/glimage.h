#ifndef __GLIMAGE_H__
#define __GLIMAGE_H__

#include "path.h"
#include "reference.h"

#include <inttypes.h>
#include <stdbool.h>

#include <SDL_opengl.h>

struct glimage {
    int refcount;

    uint16_t w;
    uint16_t h;

    uint16_t s_w; // slices wide
    uint16_t s_h; // slices tall
    GLuint slices[IMAGE_MAX_SLICES];

    uint32_t cpu_time; // ms
    uint32_t gl_time;

    char path[MAX_PATH_LENGTH];
};

struct glimage * glimage_from_cpuimage(struct cpuimage *i);
void glimage_draw(struct glimage *gl, float cx, float cy, float width, float height, float pixel_size, bool multidraw);

void glimage_incr(void *gl);
void glimage_decr(void *gl);

static inline void glimage_decr_q(struct glimage *i) {
    ref_queue_decr((void*)(i), glimage_decr);
}

#endif
