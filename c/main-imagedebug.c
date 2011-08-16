#include "image.h"

#include <stdio.h>
#include <err.h>
#include <inttypes.h>

int main(int argc, char **argv) {
    struct cpuimage cpu;

    if ( !image_load_from_disk(argv[1], &cpu) )
        errx(1, "Couldn't load image from disk");

    char path[50];
    for (int sx = 0; sx < cpu.s_w; sx++)
        for (int sy = 0; sy < cpu.s_h; sy++) {
            uint8_t *slicebase = (uint8_t*)cpu.slices + (sx+sy*cpu.s_w)*IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE*4;

            snprintf(path, 50, "image-%d.%d.ppm", sx, sy);

            printf("writing %s...\n", path);

            FILE *fh = fopen(path, "wb");
            fprintf(fh, "P6 %d %d %d ", IMAGE_SLICE_SIZE, IMAGE_SLICE_SIZE, 255);
            int x = 0;
            int y = 0;
            for (int i = 0; i < IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE; i++) {
                uint8_t r = slicebase[i*4+2];
                uint8_t g = slicebase[i*4+1];
                uint8_t b = slicebase[i*4+0];
                uint8_t a = slicebase[i*4+3];

                if ( a < 255 ) {
                    uint8_t base_checker = ((x/16 % 2) + (y/16 % 2)) % 2 ? 192 : 64;
                    r = ((int)r * a + (int)base_checker * (255-a)) >> 8;
                    g = ((int)g * a + (int)base_checker * (255-a)) >> 8;
                    b = ((int)b * a + (int)base_checker * (255-a)) >> 8;
                }

                fprintf(fh, "%c%c%c", r, g, b);

                x++;
                if ( x >= IMAGE_SLICE_SIZE ) {
                    x = 0;
                    y++;
                }
            }

            fclose(fh);
        }

    return 0;
}
