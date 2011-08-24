#if ENABLE_PNM

#include "image-pnm.h"

#include <err.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

// TODO: ew
bool cpuimage_load_from_ram_pnm(void *orig, int len, struct cpuimage *i) {
    uint32_t width, height, maxval, version;
    uint8_t *ptr = (uint8_t*) orig;

#define NEEDCOUNT(n) if ( len < (n) ) { \
    warnx("Need %d more bytes in PNM", (n)); \
    return false; \
}

#define EATCHAR(ch) if ( len < 1 || *ptr != (ch) ) { \
    warnx("Expected '%c' in PNM", ch); \
    return false; \
} else { \
    ptr++; \
    len--; \
}

#define EATWHITESPACE while ( len >= 1 && isspace(*ptr) ) { ptr++; len--; }
#define EATCOMMENTS while ( len >= 1 ) { \
    if ( isspace(*ptr) ) { \
        ptr++; len--; \
    } else if ( *ptr == '#' ) { \
        while ( len >= 1 && *ptr != '\n' ) { ptr++; len--; } \
    } else { \
        break; \
    } \
}

#define EATNUMBER(into) if ( len < 1 || !isdigit(*ptr) ) { \
    warnx("Expected number in PNM (got %c)", *ptr); \
    return false; \
} else { \
    int acc = 0; \
    while ( len >= 1 && isdigit(*ptr) ) { \
        acc = acc*10 + (*ptr - '0'); \
        ptr++; len--; \
    } \
    (into) = acc; \
}

    ////////////////////////////////////////
    // Parse header

    EATCHAR('P');

    NEEDCOUNT(1);
    version = (*ptr)-'0';
    if ( version < 1 || version > 6 ) {
        warnx("Expected version between 1 and 6 (got %d)", version);
        return false;
    }
    ptr++; len--;

    EATCOMMENTS;
    EATNUMBER(width);
    EATCOMMENTS;
    EATNUMBER(height);
    
    if ( version != 1 && version != 4 ) {
        // not PBM, has maxval
        EATCOMMENTS;
        EATNUMBER(maxval);
    } else {
        maxval = 1;
    }
    
    if ( len < 1 || !isspace(*ptr) ) {
        warnx("Expected whitespace after header");
        return false;
    }
    len--;

    ////////////////////////////////////////
    // Prepare image container

    // XXX: on error later, free this
    if ( !cpuimage_setup_cpu_wh(i, width, height) ) {
        warnx("Couldn't set w/h for PNM file\n");
        return false;
    }
    i->is_bgra = false;

    ////////////////////////////////////////
    // Load image data

    uint8_t r,g,b;
    int temp = 0, left;

    // extra space at the right of the last slice
    int extra_width = i->s_w*IMAGE_SLICE_SIZE - width;
    int extra_width_starts = IMAGE_SLICE_SIZE - extra_width;

    // in bytes
    extra_width *= 4;
    extra_width_starts *= 4;

    for (int y = 0; y < height; y++) {
        int sy = y/IMAGE_SLICE_SIZE;
        uint8_t *slice, *row, *at = NULL;

        left = 0; // Binary PBM: aligned to row boundaries

        for (int x = 0; x < width; x++) {
            int sx = x/IMAGE_SLICE_SIZE;

            slice = i->slices + IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE*4*(sx + sy*i->s_w);
            row = slice + IMAGE_SLICE_SIZE*4*(y-sy*IMAGE_SLICE_SIZE);
            at = row + 4*(x-sx*IMAGE_SLICE_SIZE);

            // TODO: for binary formats, verify the data length each row, not each pixel

            // TODO: flip the x loop inside this if statement? or pray for a good compiler?
            switch ( version ) {
                case 1: // ASCII PBM
                    EATWHITESPACE;
                    EATNUMBER(temp);
                    r=g=b = temp == 0 ? 0 : 255;
                    break;

                case 2: // ASCII PGM
                    EATWHITESPACE;
                    EATNUMBER(temp);
                    r=g=b = temp*255/maxval;
                    break;

                case 3: // ASCII PPM
                    EATWHITESPACE;
                    EATNUMBER(temp);
                    r = temp*255/maxval;
                    EATWHITESPACE;
                    EATNUMBER(temp);
                    g = temp*255/maxval;
                    EATWHITESPACE;
                    EATNUMBER(temp);
                    b = temp*255/maxval;
                    break;

                case 4: // Binary PBM
                    if ( left ) {
                        r=g=b = (temp & 0x80) ? 255 : 0;
                        temp = (temp << 1) & 0xff;
                        left--;
                    } else {
                        if ( len < 1 ) {
                            warnx("PBM appears truncated");
                            return false;
                        }
                        left = 7;
                        temp = *ptr++;
                        len--;

                        r=g=b = (temp & 0x80) ? 255 : 0;
                        temp = (temp << 1) & 0xff;
                    }
                    break;

                case 5: // Binary PGM
                    if ( len < 1 ) {
                        warnx("PGM appears truncated");
                        return false;
                    }
                    if ( maxval == 255 ) {
                        r=g=b = *ptr++;
                    } else if ( maxval == 65535 ) {
                        if ( len < 2 ) {
                            warnx("PGM appears truncated");
                            return false;
                        }
                        r=g=b = *ptr++; ptr++; // XXX something is horriby wrong
                    } else if ( maxval < 255 ) {
                        r=g=b = (*ptr++)*255/maxval;
                    } else if ( maxval < 65535 ) {
                        if ( len < 2 ) {
                            warnx("PGM appears truncated");
                            return false;
                        }
                        r=g=b = (*ptr++)*65535/maxval; ptr++;
                    } else {
                        warnx("PGM has invalid maxval: %d", maxval);
                        return false;
                    }
                    break;

                case 6: // Binary PPM
                    if ( len < 3 ) {
                        warnx("PPM appears truncated");
                        return false;
                    }
                    if ( maxval == 255 ) {
                        r = *ptr++;
                        g = *ptr++;
                        b = *ptr++;
                    } else if ( maxval == 65535 ) {
                        if ( len < 6 ) {
                            warnx("PPM appears truncated");
                            return false;
                        }
                        r = *ptr++; ptr++;
                        g = *ptr++; ptr++;
                        b = *ptr++; ptr++;
                    } else if ( maxval < 255 ) {
                        r = (*ptr++)*255/maxval;
                        g = (*ptr++)*255/maxval;
                        b = (*ptr++)*255/maxval;
                    } else if ( maxval < 65535 ) {
                        if ( len < 6 ) {
                            warnx("PPM appears truncated");
                            return false;
                        }
                        r = (*ptr++)*65535/maxval; ptr++;
                        g = (*ptr++)*65535/maxval; ptr++;
                        b = (*ptr++)*65535/maxval; ptr++;
                    } else {
                        warnx("PPM has invalid maxval: %d", maxval);
                        return false;
                    }
                    break;

                default:
                    errx(1, "PNM internal error (not reached)");
            }

            // TODO: OMGWTFHAX??!?!?! (endianness?)
            *at++ = g;
            *at++ = b;
            *at++ = r;
            *at++ = 255; // a
        }

        if ( extra_width && at )
            memset(at, 0, extra_width);
    }

    // ignore trailing data:
    //     as defined for ascii images,
    //     only use the first raster of binary images

    int clear_rows = i->s_h*IMAGE_SLICE_SIZE - height;
    int start_clear_row = IMAGE_SLICE_SIZE - clear_rows;
    int clear_bytes = clear_rows * IMAGE_SLICE_SIZE*4;
    for (int sx = 0; sx < i->s_w; sx++)
        memset(i->slices + (IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE*4)*(sx+(i->s_h-1)*i->s_w) + start_clear_row*IMAGE_SLICE_SIZE*4, 0, clear_bytes);

    return true;
}

#endif

