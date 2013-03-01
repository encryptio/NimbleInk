#include "image.h"
#include "reference.h"

#include <stdio.h>
#include <err.h>
#include <inttypes.h>

int main(int argc, char **argv) {
    struct cpuimage *cpu;

    ref_release_pool();

    if ( (cpu = cpuimage_from_disk(argv[1])) == NULL )
        errx(1, "Couldn't load image from disk");
    cpuimage_incr(cpu);

    ref_release_pool();

    char path[50];
    for (int sx = 0; sx < cpu->s_w; sx++)
        for (int sy = 0; sy < cpu->s_h; sy++) {
            uint8_t *slicebase = (uint8_t*)cpu->slices + (sx+sy*cpu->s_w)*IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE*4;

            snprintf(path, 50, "image-%d.%d.ppm", sx, sy);

            printf("writing %s...\n", path);

            FILE *fh = fopen(path, "wb");
            fprintf(fh, "P6 %d %d %d ", IMAGE_SLICE_SIZE, IMAGE_SLICE_SIZE, 255);
            int x = 0;
            int y = 0;
            for (int i = 0; i < IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE; i++) {
                uint8_t r,g,b,a;

                switch ( cpu->pf ) {
                    case CPUIMAGE_RGBA:
                        r = slicebase[i*4  ];
                        g = slicebase[i*4+1];
                        b = slicebase[i*4+2];
                        a = slicebase[i*4+3];
                        break;

                    case CPUIMAGE_BGRA:
                        b = slicebase[i*4  ];
                        g = slicebase[i*4+1];
                        r = slicebase[i*4+2];
                        a = slicebase[i*4+3];
                        break;

                    case CPUIMAGE_RGB:
                        r = slicebase[i*3  ];
                        g = slicebase[i*3+1];
                        b = slicebase[i*3+2];
                        a = 0xff;
                        break;

                    case CPUIMAGE_BGR:
                        b = slicebase[i*3  ];
                        g = slicebase[i*3+1];
                        r = slicebase[i*3+2];
                        a = 0xff;
                        break;

                    case CPUIMAGE_GRAY:
                        r = g = b = slicebase[i];
                        a = 0xff;
                        break;
                }

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

    cpuimage_decr_q(cpu);
    ref_release_pool();

    return 0;
}
