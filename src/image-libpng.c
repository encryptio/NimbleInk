#if ENABLE_LIBPNG

#include "image.h"
#include "image-libpng.h"

#include <string.h>

#include <png.h>

struct png_reading_status { void *ptr; int len; int at; };

static void image_libpng_read(png_structp png_ptr, png_bytep out_bytes, png_size_t want) {
    // todo: how do i handle error returns?

    if ( png_get_io_ptr(png_ptr) == NULL )
        return;

    struct png_reading_status *rs = (struct png_reading_status *) png_get_io_ptr(png_ptr);

    int got = want;
    if ( got > rs->len - rs->at )
        got = rs->len - rs->at;

    if ( got )
        memcpy(out_bytes, rs->ptr + rs->at, got);

    rs->at += got;

    return;
}


bool cpuimage_load_from_ram_libpng(void *ptr, int len, struct cpuimage *i) {
    if ( !png_check_sig(ptr, len) )
        return false;

    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytepp row_pointers = NULL;
    bool ret = true;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if ( png_ptr == NULL ) {
        ret = false;
        goto DESTROY;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if ( info_ptr == NULL ) {
        ret = false;
        goto DESTROY;
    }

    struct png_reading_status read_status = { ptr, len, 0 };
    png_set_read_fn(png_ptr, &read_status, image_libpng_read);

    if ( setjmp(png_jmpbuf(png_ptr)) ) {
        ret = false;
        goto DESTROY;
    }

    png_read_png(png_ptr, info_ptr,
            PNG_TRANSFORM_STRIP_16 // 16 bit channels -> 8 bit channels
            | PNG_TRANSFORM_PACKING // 1,2,4 bit channels -> 8 bit channels
            | PNG_TRANSFORM_EXPAND, // expand to RGB or RGBA
            NULL);

    row_pointers = png_get_rows(png_ptr, info_ptr);

    int y_range, x_range;

    int channel_count;
    enum cpuimage_pixel_format pixfmt;

    switch (png_get_color_type(png_ptr, info_ptr)) {
        case PNG_COLOR_TYPE_RGBA:
            channel_count = 4;
            pixfmt = CPUIMAGE_RGBA;
            break;

        case PNG_COLOR_TYPE_RGB:
            channel_count = 3;
            pixfmt = CPUIMAGE_RGB;
            break;

        case PNG_COLOR_TYPE_GRAY:
            channel_count = 1;
            pixfmt = CPUIMAGE_GRAY;
            break;

        case PNG_COLOR_TYPE_GRAY_ALPHA:
            channel_count = 2;
            pixfmt = CPUIMAGE_GRAYA;
            break;

        default:
            fprintf(stderr, "Cannot load the unsupported png color type %d\n", png_get_color_type(png_ptr, info_ptr));
            ret = false;
            goto DESTROY;
    }

    cpuimage_setup_cpu_wh(i, png_get_image_width(png_ptr, info_ptr), png_get_image_height(png_ptr, info_ptr), pixfmt);

    for (int sy = 0; sy < i->s_h; sy++) {
        y_range = IMAGE_SLICE_SIZE;
        if ( (sy+1)*IMAGE_SLICE_SIZE-1 >= i->h )
            y_range = i->h - sy*IMAGE_SLICE_SIZE;

        for (int sx = 0; sx < i->s_w; sx++) {
            x_range = IMAGE_SLICE_SIZE;
            if ( (sx+1)*IMAGE_SLICE_SIZE-1 >= i->w )
                x_range = i->w - sx*IMAGE_SLICE_SIZE;

            for (int y = 0; y < y_range; y++) {
                uint8_t *slice = i->slices + IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE*(sx + sy*i->s_w)*channel_count;
                uint8_t *row = slice + IMAGE_SLICE_SIZE * y * channel_count;
                memcpy(row, row_pointers[y+sy*IMAGE_SLICE_SIZE] + sx*IMAGE_SLICE_SIZE*channel_count, x_range*channel_count);
            }
        }
    }

DESTROY:
    if ( png_ptr != NULL )
        png_destroy_read_struct(&png_ptr, NULL, NULL);

    return ret;
}

#endif /* ENABLE_LIBPNG */
