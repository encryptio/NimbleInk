#include "image.h"

#include "image-libjpeg.h"
#include "image-giflib.h"
#include "image-sdl_image.h"
#include "image-pnm.h"

#include "archive.h"
#include "filetype.h"

#include <SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <inttypes.h>

struct cpuimage * cpuimage_from_disk(char *path) {
    FILE *fh = fopen(path, "rb");

    uint8_t *buf;
    int alloced = 8192;
    int used = 0;

    if ( (buf = malloc(alloced)) == NULL )
        err(1, "Couldn't allocate space for image load from disk");

    size_t did_read;
    while ( (did_read = fread(buf+used, 1, alloced-used, fh)) > 0 ) {
        used += did_read;
        if ( used == alloced ) {
            alloced *= 2;

            if ( alloced > IMAGE_MAX_FILESIZE ) {
                warnx("Image in %s is too big to be loaded (max size %d bytes)", path, IMAGE_MAX_FILESIZE);
                fclose(fh);
                free(buf);
                return false;
            }

            if ( (buf = realloc(buf, alloced)) == NULL )
                err(1, "Couldn't realloc space for image load from disk");
        }
    }

    fclose(fh);

    struct cpuimage *ret = cpuimage_from_ram(buf, used);

    free(buf);

    return ret;
}

struct cpuimage * cpuimage_from_ram(void *ptr, int len) {
    uint32_t start = SDL_GetTicks();

    struct cpuimage *i;
    if ( (i = calloc(1, sizeof(struct cpuimage))) == NULL )
        err(1, "Couldn't allocate space for cpuimage");
    i->refcount = 1;
    cpuimage_decr_q(i);

    bool ret = false;

#if ENABLE_LIBJPEG
    if ( !ret && len >= FILETYPE_MAGIC_BYTES && ft_is_jpg((uint8_t*) ptr) )
        ret = cpuimage_load_from_ram_libjpeg(ptr, len, i);
#endif
#if ENABLE_GIFLIB
    if ( !ret && len >= FILETYPE_MAGIC_BYTES && ft_is_gif((uint8_t*) ptr) )
        ret = cpuimage_load_from_ram_giflib(ptr, len, i);
#endif
#if ENABLE_PNM
    if ( !ret && len >= FILETYPE_MAGIC_BYTES && ft_is_pnm((uint8_t*) ptr) )
        ret = cpuimage_load_from_ram_pnm(ptr, len, i);
#endif
#if ENABLE_SDL_IMAGE
    if ( !ret )
        ret = cpuimage_load_from_ram_sdl_image(ptr, len, i);
#endif

    i->cpu_time = SDL_GetTicks() - start;

    snprintf(i->path, MAX_PATH_LENGTH, "Address %p length %d", ptr, len);

    return ret ? i : NULL;
}

bool cpuimage_setup_cpu_wh(struct cpuimage *i, int w, int h) {
    i->w = w;
    i->h = h;
    i->s_w = (w + IMAGE_SLICE_SIZE - 1) / IMAGE_SLICE_SIZE;
    i->s_h = (h + IMAGE_SLICE_SIZE - 1) / IMAGE_SLICE_SIZE;

    if ( i->s_w * i->s_h > IMAGE_MAX_SLICES ) {
        warnx("Too many slices. wanted %d (=%dx%d) slices but only have structure room for %d", i->s_w*i->s_h, i->s_w, i->s_h, IMAGE_MAX_SLICES);
        return false;
    }

    if ( (i->slices = malloc(4 * IMAGE_SLICE_SIZE * IMAGE_SLICE_SIZE * i->s_w * i->s_h)) == NULL )
        err(1, "Couldn't malloc space for image");

#if DEBUG_RANDOMIZE_SLICES
    for (int j = 0; j < 4*IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE*i->s_w*i->s_h/sizeof(long); j++)
        ((long*) i->slices)[j] = random();
#endif

    return true;
}

static void cpuimage_free(struct cpuimage *i) {
    if ( i->slices )
        free(i->slices);
    free(i);
}

void cpuimage_incr(void *i) {
    ((struct cpuimage *)i)->refcount++;
}

void cpuimage_decr(void *i) {
    if ( !( --((struct cpuimage *) i)->refcount ) )
        cpuimage_free((struct cpuimage *) i);
}

