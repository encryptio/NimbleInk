#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <inttypes.h>
#include <stdbool.h>

#include <SDL_opengl.h>

#define IMAGE_SLICE_SIZE 512
// TODO: add thumbnail slices

#define IMAGE_MAX_SLICES 256
#define IMAGE_MAX_PATH_LENGTH 256

// slice order is top down, left to right (x varies fastest)

struct cpuimage {
    uint16_t w;
    uint16_t h;
    
    uint16_t s_w; // slices wide
    uint16_t s_h; // slices tall

    char path[IMAGE_MAX_PATH_LENGTH];

    void *slices; // always RGBA or BGRA, in slice order

    bool is_bgra; // true if the above is formatted BGRA, false if RGBA

    uint32_t load_time; // ms
    uint32_t cpu_time;
};

struct glimage {
    uint16_t w;
    uint16_t h;

    uint16_t s_w; // slices wide
    uint16_t s_h; // slices tall

    char path[IMAGE_MAX_PATH_LENGTH];

    GLuint slices[IMAGE_MAX_SLICES];

    uint32_t load_time; // ms
    uint32_t cpu_time;
    uint32_t gl_time;
};

bool image_load_from_disk(char *path, struct cpuimage *i);
bool image_load_from_ram(void *ptr, int len, struct cpuimage *i);
bool image_cpu2gl(struct cpuimage *i, struct glimage *gl);
void image_gl_destroy(struct glimage *gl);
void image_cpu_destroy(struct cpuimage *i);
void image_draw(struct glimage *gl, float x1, float y1, float x2, float y2);

#endif
