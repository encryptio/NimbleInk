#if ENABLE_GIFLIB

#include "image-giflib.h"
#include "inklog.h"
#define INKLOG_MODULE "image-giflib"

#include <string.h>
#include <err.h>
#include <stdio.h>

#include "gif_lib.h"

struct gif_reading_status { void *ptr; int len; int at; };

static int image_giflib_read(GifFileType *gif, GifByteType *into, int want) {
    struct gif_reading_status *ex = (struct gif_reading_status *) gif->UserData;

    int got = want;
    if ( got > ex->len - ex->at )
        got = ex->len - ex->at;

    if ( got )
        memcpy(into, ex->ptr + ex->at, got);

    ex->at += got;

    return got;
}

bool cpuimage_load_from_ram_giflib(void *ptr, int len, struct cpuimage *i) {
    struct gif_reading_status ex = { ptr, len, 0 };
    bool ret = false;

    GifFileType *gif = DGifOpen(&ex, image_giflib_read);

    if ( gif == NULL ) {
        inklog(LOG_WARNING, "Couldn't open gif, error code %d", GifLastError());
        goto RETURN;
    }

    // XXX: possibly a bug in giflib wrt interlaced images
    // http://www.mail-archive.com/debian-bugs-dist@lists.debian.org/msg929400.html
    if ( DGifSlurp(gif) != GIF_OK ) {
        inklog(LOG_WARNING, "Couldn't slurp the gif file, error code %d", GifLastError());
        goto DESTROY;
    }

    if ( !cpuimage_setup_cpu_wh(i, gif->SWidth, gif->SHeight, CPUIMAGE_BGRA) ) {
        inklog(LOG_ERR, "Couldn't set w/h for gif file");
        goto DESTROY;
    }

    struct SavedImage *img = gif->SavedImages;
    ColorMapObject *map = img->ImageDesc.ColorMap;

    if ( map == NULL )
        map = gif->SColorMap;

    if ( map == NULL ) {
        inklog(LOG_ERR, "gif map is NULL");
        goto DESTROY;
    }

    for (int y = 0; y < gif->SHeight; y++) {
        int sy = y/IMAGE_SLICE_SIZE;
        int x = 0;
        for (int sx = 0; sx < i->s_w; sx++) {
            uint8_t *slice = (uint8_t*)i->slices + IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE*4*(sx + sy*i->s_w);
            uint8_t *at = slice + (y-sy*IMAGE_SLICE_SIZE)*IMAGE_SLICE_SIZE*4;
            for (int z = 0; z < IMAGE_SLICE_SIZE; z++) {
                if ( x < gif->SWidth ) {
                    GifColorType *c = &(map->Colors[img->RasterBits[x+y*gif->SWidth]]);
                    *at++ = c->Blue;
                    *at++ = c->Green;
                    *at++ = c->Red;
                    *at++ = 255; // TODO: gif transparency
                } else {
                    *at++ = 0; // b
                    *at++ = 0; // g
                    *at++ = 0; // r
                    *at++ = 0; // a
                }
                x++;
            }
        }
    }

    ret = true;

DESTROY:
    if ( DGifCloseFile(gif) != GIF_OK )
        inklog(LOG_CRIT, "Couldn't DGifCloseFile(%p)", gif);

RETURN:

    return ret;
}

#endif
