#if ENABLE_SDL_IMAGE

#include "image-sdl_image.h"

#include <SDL.h>
#include <SDL_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <inttypes.h>

static bool cpuimage_load_from_surface(SDL_Surface *surface, struct cpuimage *i);

bool cpuimage_load_from_ram_sdl_image(void *ptr, int len, struct cpuimage *i) {
    SDL_Surface *surface;

    if ( !(surface = IMG_Load_RW(SDL_RWFromMem(ptr, len), 1)) ) {
        warnx("Couldn't load image from memory: %s", IMG_GetError());
        return false;
    }

    bool ret = cpuimage_load_from_surface(surface, i);

    SDL_FreeSurface(surface);

    return ret;
}

static bool cpuimage_load_from_surface(SDL_Surface *surface, struct cpuimage *i) {
    GLint nOfColors = surface->format->BytesPerPixel;

    if ( nOfColors != 4 && nOfColors != 3 ) {
        SDL_Surface *new = SDL_DisplayFormatAlpha(surface);
        if ( !new )
            errx(1, "Couldn't convert surface to display format: %s", SDL_GetError());
        bool ret = cpuimage_load_from_surface(new, i);
        SDL_FreeSurface(new);
        return ret;
    }

    enum cpuimage_color_space color_space = (surface->format->Rmask == 0x000000ff) ? CPUIMAGE_RGBA : CPUIMAGE_BGRA;
    if ( !cpuimage_setup_cpu_wh(i, surface->w, surface->h, color_space) )
        return false;

    for (int sy = 0; sy < i->s_h; sy++)
        for (int sx = 0; sx < i->s_w; sx++) {
            void *slice_base = i->slices + (4*IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE)*(sx + sy*i->s_w);

            int surf_x_base = sx*IMAGE_SLICE_SIZE;
            int surf_y_base = sy*IMAGE_SLICE_SIZE;

            int end_copy_row = i->h - surf_y_base; // exclusive, not inclusive
            if ( end_copy_row > IMAGE_SLICE_SIZE )
                end_copy_row = IMAGE_SLICE_SIZE;

            int end_copy_column = i->w - surf_x_base; // exclusive
            if ( end_copy_column > IMAGE_SLICE_SIZE )
                end_copy_column = IMAGE_SLICE_SIZE;

            if ( nOfColors == 4 ) {
                // already RGBA or BGRA

                for (int y = 0; y < end_copy_row; y++)
                    memcpy(slice_base + 4*y*IMAGE_SLICE_SIZE, ((void*)surface->pixels) + (surf_y_base + y)*surface->pitch + 4*surf_x_base, 4*end_copy_column);
            } else if ( nOfColors == 3 ) {
                // RGB or BGR, need to add the alpha channel

                uint8_t  *sl1   = (uint8_t*)  slice_base;
                uint32_t *sl4   = (uint32_t*) slice_base;
                uint8_t  *surf1 = (uint8_t*)  surface->pixels;

                for (int y = 0; y < end_copy_row; y++) {
                    for (int x = 0; x < end_copy_column; x++) {
                        // TODO: endianness check
                        sl1[(x+y*IMAGE_SLICE_SIZE)*4 + 0] = surf1[((x+surf_x_base)*3+(y+surf_y_base)*surface->pitch) + 0];
                        sl1[(x+y*IMAGE_SLICE_SIZE)*4 + 1] = surf1[((x+surf_x_base)*3+(y+surf_y_base)*surface->pitch) + 1];
                        sl1[(x+y*IMAGE_SLICE_SIZE)*4 + 2] = surf1[((x+surf_x_base)*3+(y+surf_y_base)*surface->pitch) + 2];
                        sl1[(x+y*IMAGE_SLICE_SIZE)*4 + 3] = 255; // alpha
                    }
                }
            } else {
                errx(1, "not reached");
            }
        }

    return true;
}

#endif /* ENABLE_SDL_IMAGE */
