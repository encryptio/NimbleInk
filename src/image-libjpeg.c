#if ENABLE_LIBJPEG

#include "image-libjpeg.h"

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

    if ( !cpuimage_setup_cpu_wh(i, cinfo.image_width, cinfo.image_height, CPUIMAGE_BGRA) ) {
        warn("Couldn't setup cpuimage");
        goto DESTROY;
    }

    cinfo.out_color_space = JCS_RGB;
    cinfo.buffered_image = false;
    //cinfo.do_block_smoothing = true; // ?
    // TODO: verify gamma correctness
    // TODO: consider changing iDCT method
    
    jpeg_start_decompress(&cinfo);

    assert(cinfo.output_components == 3); // TODO: will this ever fail?

    if ( (scanlines = malloc(sizeof(JSAMPLE) * cinfo.output_width * cinfo.output_components * IMAGE_LIBJPEG_SCANLINES_PER_LOOP)) == NULL )
        err(1, "Couldn't allocate space for scanlines");

    JSAMPROW input[IMAGE_LIBJPEG_SCANLINES_PER_LOOP];
    for (int i = 0; i < IMAGE_LIBJPEG_SCANLINES_PER_LOOP; i++)
        input[i] = scanlines + i*(sizeof(JSAMPLE)*cinfo.output_width*cinfo.output_components);

    int y = 0;
    while ( cinfo.output_scanline < cinfo.output_height ) {
        int read = jpeg_read_scanlines(&cinfo, input, IMAGE_LIBJPEG_SCANLINES_PER_LOOP);
        for (int dy = 0; dy < read; dy++) {
            int sy = y/IMAGE_SLICE_SIZE;

            // number of bytes into the slice
            int yoff = (y-sy*IMAGE_SLICE_SIZE) * 4 * IMAGE_SLICE_SIZE;
            int xleft = cinfo.output_width;

            for (int sx = 0; sx < i->s_w; sx++) {
                uint8_t *slice = i->slices + IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE*(sx + sy*i->s_w)*4;

                uint8_t *this = slice + yoff;
                for (int in_x = 0; in_x < IMAGE_SLICE_SIZE; in_x++) {
                    // TODO: endianness
                    this[in_x*4  ] = input[dy][(sx*IMAGE_SLICE_SIZE+in_x)*3+2];
                    this[in_x*4+1] = input[dy][(sx*IMAGE_SLICE_SIZE+in_x)*3+1];
                    this[in_x*4+2] = input[dy][(sx*IMAGE_SLICE_SIZE+in_x)*3  ];
                    this[in_x*4+3] = 255;

                    xleft--;
                    if ( xleft == 0 && in_x < IMAGE_SLICE_SIZE-1 ) {
                        memset(this + (in_x+1)*4, 0, (IMAGE_SLICE_SIZE-in_x-1)*4);
                        break;
                    }
                }
            }
            y++;
        }
    }

    // TODO: are we clearing the first row after the image correctly?
    int clear_rows = i->s_h*IMAGE_SLICE_SIZE - y;
    int start_clear_row = IMAGE_SLICE_SIZE - clear_rows;
    int clear_bytes = clear_rows * IMAGE_SLICE_SIZE*4;
    for (int sx = 0; sx < i->s_w; sx++)
        memset(i->slices + (IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE*4)*(sx+(i->s_h-1)*i->s_w) + start_clear_row*IMAGE_SLICE_SIZE*4, 0, clear_bytes);

    jpeg_finish_decompress(&cinfo);

    ret = true;

DESTROY:
    free(scanlines);
    jpeg_destroy_decompress(&cinfo);

    return ret;
}

#endif /* ENABLE_LIBJPEG */

