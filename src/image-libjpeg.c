#if ENABLE_LIBJPEG

#include "image-libjpeg.h"
#include "inklog.h"
#define INKLOG_MODULE "image-libjpeg"

#include <string.h>
#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include "jpeglib.h"

// note: must divide IMAGE_SLICE_SIZE
#define IMAGE_LIBJPEG_SCANLINES_PER_LOOP 4

// note: this code uses stdbool definitions for the jpeg booleans and
// assumes they are source compatible

bool cpuimage_load_from_ram_libjpeg(void *ptr, int len, struct cpuimage *i) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    bool ret = false;
    uint8_t *scanlines = NULL;

    cinfo.err = jpeg_std_error(&jerr); // TODO: replace with non-terminating error handler
    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, ptr, len);

    jpeg_read_header(&cinfo, true);

    enum cpuimage_pixel_format target_pixfmt;
    int slice_channels;
    switch ( cinfo.jpeg_color_space ) {
        case JCS_GRAYSCALE:
            target_pixfmt = CPUIMAGE_GRAY;
            slice_channels = 1;
            break;

        case JCS_YCbCr:
            cinfo.out_color_space = JCS_RGB;
        case JCS_RGB:
            target_pixfmt = CPUIMAGE_RGB;
            slice_channels = 3;
            break;

        case JCS_YCCK:
            cinfo.out_color_space = JCS_CMYK;
        case JCS_CMYK:
            target_pixfmt = CPUIMAGE_RGB;
            slice_channels = 3;
            break;

        case JCS_UNKNOWN:
        default:
            warn("[jpeg] Couldn't load jpeg: unknown color space");
            goto DESTROY;
    }

    if ( !cpuimage_setup_cpu_wh(i, cinfo.image_width, cinfo.image_height, target_pixfmt) ) {
        warn("Couldn't setup cpuimage");
        goto DESTROY;
    }

    cinfo.buffered_image = false;
    //cinfo.do_block_smoothing = true; // ?
    // TODO: verify gamma correctness
    // TODO: consider changing iDCT method

    jpeg_start_decompress(&cinfo);

    switch ( cinfo.jpeg_color_space ) {
        case JCS_GRAYSCALE:
            assert(cinfo.output_components == 1);
            break;

        case JCS_RGB:
        case JCS_YCbCr:
            assert(cinfo.output_components == 3);
            break;

        case JCS_CMYK:
        case JCS_YCCK:
            assert(cinfo.output_components == 4);
            break;

        case JCS_UNKNOWN:
        default:
            errx(1, "not reached");
    }

    if ( (scanlines = malloc(sizeof(JSAMPLE) * cinfo.output_width * cinfo.output_components * IMAGE_LIBJPEG_SCANLINES_PER_LOOP)) == NULL ) {
        inklog(LOG_CRIT, "Couldn't allocate space for scanlines");
        goto DESTROY;
    }

    JSAMPROW input[IMAGE_LIBJPEG_SCANLINES_PER_LOOP];
    for (int i = 0; i < IMAGE_LIBJPEG_SCANLINES_PER_LOOP; i++)
        input[i] = scanlines + i*(sizeof(JSAMPLE)*cinfo.output_width*cinfo.output_components);

    int y = 0;
    while ( cinfo.output_scanline < cinfo.output_height ) {
        int read = jpeg_read_scanlines(&cinfo, input, IMAGE_LIBJPEG_SCANLINES_PER_LOOP);
        for (int dy = 0; dy < read; dy++) {
            int sy = y/IMAGE_SLICE_SIZE;

            for (int sx = 0; sx < i->s_w; sx++) {
                uint8_t *slice = i->slices + IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE*(sx + sy*i->s_w)*slice_channels;
                uint8_t *row = slice + (y-sy*IMAGE_SLICE_SIZE)*IMAGE_SLICE_SIZE*slice_channels;

                int x_pixels = IMAGE_SLICE_SIZE;
                if ( sx == i->s_w-1 )
                    x_pixels = i->w - (i->s_w-1)*IMAGE_SLICE_SIZE;

                int x_offset = sx*IMAGE_SLICE_SIZE;

                switch ( cinfo.jpeg_color_space ) {
                    case JCS_GRAYSCALE:
                    case JCS_YCbCr: // transformed to RGB by libjpeg
                    case JCS_RGB:
                        // copy unchanged into target. TODO: load directly into cpuimage buffer
                        memcpy(row, input[dy] + x_offset*cinfo.output_components, x_pixels * slice_channels);
                        break;

                    case JCS_YCCK: // transformed to CMYK by libjpeg
                    case JCS_CMYK:
                        // transform into RGB
                        for (int x = 0; x < x_pixels; x++) {
                            int c = input[dy][(x_offset+x)*cinfo.output_components + 0];
                            int m = input[dy][(x_offset+x)*cinfo.output_components + 1];
                            int y = input[dy][(x_offset+x)*cinfo.output_components + 2];
                            int k = input[dy][(x_offset+x)*cinfo.output_components + 3];

                            row[x*slice_channels + 0] = c * k / 255; // r
                            row[x*slice_channels + 1] = m * k / 255; // g
                            row[x*slice_channels + 2] = y * k / 255; // b
                        }
                        break;

                    case JCS_UNKNOWN:
                    default:
                        errx(1, "not reached");
                }
            }

            y++;
        }
    }

    jpeg_finish_decompress(&cinfo);

    ret = true;

DESTROY:
    if ( scanlines != NULL )
        free(scanlines);
    jpeg_destroy_decompress(&cinfo);

    return ret;
}

#endif /* ENABLE_LIBJPEG */
