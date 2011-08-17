#include "image.h"

#include "image-libjpeg.h"
#include "image-sdl_image.h"

#include "archive.h"
#include "filetype.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <inttypes.h>

bool image_multidraw = true;

bool image_load_from_disk(char *path, struct cpuimage *i) {
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

    bool ret = image_load_from_ram(buf, used, i);

    free(buf);

    return ret;
}

bool image_load_from_ram(void *ptr, int len, struct cpuimage *i) {
    uint32_t start = SDL_GetTicks();

    bool ret;

    if ( false && len > 8 && ft_is_jpg((uint8_t*) ptr) ) {
        ret = image_load_from_ram_libjpeg(ptr, len, i);
    } else {
        ret = image_load_from_ram_sdl_image(ptr, len, i);
    }

    i->load_time = SDL_GetTicks() - start;

    snprintf(i->path, MAX_PATH_LENGTH, "Address %p length %d", ptr, len);

    return ret;
}

bool image_cpu2gl(struct cpuimage *i, struct glimage *gl) {
    uint32_t start = SDL_GetTicks();

    // copy the unchanged parts of the structure
    memcpy(gl->path, i->path, MAX_PATH_LENGTH);
    gl->w = i->w;
    gl->h = i->h;
    gl->s_w = i->s_w;
    gl->s_h = i->s_h;
    gl->load_time = i->load_time;
    gl->cpu_time = i->cpu_time;

    // create the texture IDs
    glGenTextures(gl->s_w * gl->s_h, (GLuint*) &(gl->slices));
    for (int t = 0; t < gl->s_w * gl->s_h; t++) {
        glBindTexture(GL_TEXTURE_2D, gl->slices[t]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

#ifdef USE_MIPMAPS
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // trilinear interpolation
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gluBuild2DMipmaps(GL_TEXTURE_2D, 4, IMAGE_SLICE_SIZE, IMAGE_SLICE_SIZE, (i->is_bgra ? GL_BGRA : GL_RGBA), GL_UNSIGNED_BYTE, i->slices + 4*IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE*t);
#else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, IMAGE_SLICE_SIZE, IMAGE_SLICE_SIZE, 0, (i->is_bgra ? GL_BGRA : GL_RGBA), GL_UNSIGNED_BYTE, i->slices + 4*IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE*t);
#endif

        // TODO: check glGetError
    }

    gl->gl_time = SDL_GetTicks() - start;
    return true;
}

void image_gl_destroy(struct glimage *gl) {
    glDeleteTextures(gl->s_w * gl->s_h, (GLuint*) &(gl->slices));
}

void image_cpu_destroy(struct cpuimage *i) {
    free(i->slices);
}

void image_draw(struct glimage *gl, float x1, float y1, float x2, float y2) {
    int slice = 0;
    float x_step = (x2-x1)/(gl->s_w - ((float) gl->s_w*IMAGE_SLICE_SIZE - gl->w)/IMAGE_SLICE_SIZE);
    float y_step = (y2-y1)/(gl->s_h - ((float) gl->s_h*IMAGE_SLICE_SIZE - gl->h)/IMAGE_SLICE_SIZE);

    glColor3f(1,1,1);

    for (int sy = 0; sy < gl->s_h; sy++) {
        for (int sx = 0; sx < gl->s_w; sx++) {
            glBindTexture(GL_TEXTURE_2D, gl->slices[slice]);

            if ( image_multidraw ) {
#ifndef MULTIDRAW_SIZE
#define MULTIDRAW_SIZE 2
#endif

#ifndef MULTIDRAW_SHARPNESS_FACTOR
#define MULTIDRAW_SHARPNESS_FACTOR 0.75
#endif

                // this code does incorrect things with alpha-blended source images
                int drawn = 0;
                for (int dx = 0; dx <= MULTIDRAW_SIZE; dx++)
                    for (int dy = 0; dy <= MULTIDRAW_SIZE; dy++)
                        if ( dx != 0 || dy != 0 ) {
                            glColor4f(1,1,1,1.0/(drawn+1));

                            glBegin(GL_QUADS);
                            glTexCoord2i(0,0); glVertex2f(x1 + x_step* sx    + dx*(1.0/MULTIDRAW_SIZE)*MULTIDRAW_SHARPNESS_FACTOR,
                                                          y1 + y_step* sy    + dy*(1.0/MULTIDRAW_SIZE)*MULTIDRAW_SHARPNESS_FACTOR);
                            glTexCoord2i(1,0); glVertex2f(x1 + x_step*(sx+1) + dx*(1.0/MULTIDRAW_SIZE)*MULTIDRAW_SHARPNESS_FACTOR,
                                                          y1 + y_step* sy    + dy*(1.0/MULTIDRAW_SIZE)*MULTIDRAW_SHARPNESS_FACTOR);
                            glTexCoord2i(1,1); glVertex2f(x1 + x_step*(sx+1) + dx*(1.0/MULTIDRAW_SIZE)*MULTIDRAW_SHARPNESS_FACTOR,
                                                          y1 + y_step*(sy+1) + dy*(1.0/MULTIDRAW_SIZE)*MULTIDRAW_SHARPNESS_FACTOR);
                            glTexCoord2i(0,1); glVertex2f(x1 + x_step* sx    + dx*(1.0/MULTIDRAW_SIZE)*MULTIDRAW_SHARPNESS_FACTOR,
                                                          y1 + y_step*(sy+1) + dy*(1.0/MULTIDRAW_SIZE)*MULTIDRAW_SHARPNESS_FACTOR);
                            glEnd();

                            drawn++;
                        }
            } else {
                // CCW from bottom left
                glBegin(GL_QUADS);
                glTexCoord2i(0,0); glVertex2f(x1 + x_step* sx,    y1 + y_step* sy   );
                glTexCoord2i(1,0); glVertex2f(x1 + x_step*(sx+1), y1 + y_step* sy   );
                glTexCoord2i(1,1); glVertex2f(x1 + x_step*(sx+1), y1 + y_step*(sy+1));
                glTexCoord2i(0,1); glVertex2f(x1 + x_step* sx,    y1 + y_step*(sy+1));
                glEnd();
            }

            slice++;
        }
    }
}

bool image_setup_cpu_wh(struct cpuimage *i, int w, int h) {
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

    return true;
}

