#include "image.h"
#include "inklog.h"
#define INKLOG_MODULE "cpuimage"

#include "image-libjpeg.h"
#include "image-giflib.h"
#include "image-libpng.h"
#include "image-sdl_image.h"
#include "image-pnm.h"

#include "archive.h"
#include "filetype.h"
#include "timing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <inttypes.h>
#include <errno.h>

static void cpuimage_incr(struct cpuimage *i);
static void cpuimage_decr(struct cpuimage *i);
static void cpuimage_decr_q(struct cpuimage *i);

struct cpuimage * cpuimage_from_disk(char *path) {
    FILE *fh = fopen(path, "rb");
    if ( !fh )
        return NULL;

    uint8_t *buf;
    int alloced = 8192;
    int used = 0;

    if ( (buf = malloc(alloced)) == NULL ) {
        inklog(LOG_WARNING, "Couldn't allocate space for image load from disk: %s", strerror(errno));
        fclose(fh);
        return NULL;
    }

    size_t did_read;
    while ( (did_read = fread(buf+used, 1, alloced-used, fh)) > 0 ) {
        used += did_read;
        if ( used == alloced ) {
            alloced *= 2;

            if ( alloced > IMAGE_MAX_FILESIZE ) {
                inklog(LOG_NOTICE, "Image in %s is too big to be loaded (max size %d bytes)", path, IMAGE_MAX_FILESIZE);
                fclose(fh);
                free(buf);
                return NULL;
            }

            if ( (buf = realloc(buf, alloced)) == NULL ) {
                inklog(LOG_NOTICE, "Couldn't realloc space for image load from disk (%s)", path);
                fclose(fh);
                free(buf);
                return NULL;
            }
        }
    }

    fclose(fh);

    struct cpuimage *ret = cpuimage_from_ram(buf, used);

    free(buf);

    return ret;
}

struct cpuimage * cpuimage_from_ram(void *ptr, int len) {
    double start = current_timestamp();

    struct cpuimage *i;
    if ( (i = calloc(1, sizeof(struct cpuimage))) == NULL ) {
        inklog(LOG_CRIT, "Couldn't allocate space for cpuimage struct");
        return NULL;
    }

    i->refcount = 1;
    i->incr = cpuimage_incr;
    i->decr = cpuimage_decr;
    i->decr_q = cpuimage_decr_q;

    i->decr_q(i);

    bool ret = false;

#if ENABLE_LIBJPEG
    if ( !ret && len >= FILETYPE_MAGIC_BYTES && ft_magic_matches_type((uint8_t*) ptr, FT_JPEG) )
        ret = cpuimage_load_from_ram_libjpeg(ptr, len, i);
#endif
#if ENABLE_GIFLIB
    if ( !ret && len >= FILETYPE_MAGIC_BYTES && ft_magic_matches_type((uint8_t*) ptr, FT_GIF) )
        ret = cpuimage_load_from_ram_giflib(ptr, len, i);
#endif
#if ENABLE_PNM
    if ( !ret && len >= FILETYPE_MAGIC_BYTES && ft_magic_matches_type((uint8_t*) ptr, FT_PNM) )
        ret = cpuimage_load_from_ram_pnm(ptr, len, i);
#endif
#if ENABLE_LIBPNG
    if ( !ret && len >= FILETYPE_MAGIC_BYTES && ft_magic_matches_type((uint8_t*) ptr, FT_PNG) )
        ret = cpuimage_load_from_ram_libpng(ptr, len, i);
#endif
#if ENABLE_SDL_IMAGE
    if ( !ret )
        ret = cpuimage_load_from_ram_sdl_image(ptr, len, i);
#endif

    i->cpu_time = current_timestamp() - start;

    snprintf(i->path, MAX_PATH_LENGTH, "Address %p length %d", ptr, len);

    return ret ? i : NULL;
}

static int cpuimage_channel_count_for_pixel_format(enum cpuimage_pixel_format pf) {
    switch (pf) {
        case CPUIMAGE_GRAY:
            return 1;
        case CPUIMAGE_GRAYA:
            return 2;
        case CPUIMAGE_RGB:
        case CPUIMAGE_BGR:
            return 3;
        case CPUIMAGE_RGBA:
        case CPUIMAGE_BGRA:
            return 4;
        default:
            errx(1, "Unknown pixel format value %d", pf);
    }
}

bool cpuimage_setup_cpu_wh(struct cpuimage *i, int w, int h, enum cpuimage_pixel_format pf) {
    i->w = w;
    i->h = h;
    i->s_w = (w + IMAGE_SLICE_SIZE - 1) / IMAGE_SLICE_SIZE;
    i->s_h = (h + IMAGE_SLICE_SIZE - 1) / IMAGE_SLICE_SIZE;
    i->pf = pf;

    int channels = cpuimage_channel_count_for_pixel_format(pf);

    if ( i->s_w * i->s_h > IMAGE_MAX_SLICES ) {
        inklog(LOG_WARNING, "Too many slices. Wanted %d (=%dx%d) slices for image of size %dx%d but only have structure room for %d", i->s_w*i->s_h, i->s_w, i->s_h, w, h, IMAGE_MAX_SLICES);
        return false;
    }

    if ( (i->slices = malloc(channels * IMAGE_SLICE_SIZE * IMAGE_SLICE_SIZE * i->s_w * i->s_h)) == NULL ) {
        inklog(LOG_WARNING, "Couldn't allocate %llu bytes for cpuimage slices", (long long unsigned) channels * IMAGE_SLICE_SIZE * IMAGE_SLICE_SIZE * i->s_w * i->s_h);
        return false;
    }

#if DEBUG_RANDOMIZE_SLICES
    for (int j = 0; j < channels*IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE*i->s_w*i->s_h; j++)
        ((uint8_t*) i->slices)[j] = random() & 0xff;
#endif

    return true;
}

static void cpuimage_free(struct cpuimage *i) {
    if ( i->slices )
        free(i->slices);
    free(i);
}

static void cpuimage_incr(struct cpuimage *i) {
    i->refcount++;
}

static void cpuimage_decr(struct cpuimage *i) {
    if ( !( --i->refcount ) )
        cpuimage_free(i);
}

static void cpuimage_decr_q(struct cpuimage *i) {
    ref_queue_decr((void*) i, (decr_fn) i->decr);
}
