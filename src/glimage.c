#include "image.h"

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

struct glimage * glimage_from_cpuimage(struct cpuimage *i) {
    uint32_t start = SDL_GetTicks();

    struct glimage *gl;
    if ( (gl = calloc(1, sizeof(struct glimage))) == NULL )
        err(1, "Couldn't allocate space for glimage");
    gl->refcount = 1;
    glimage_decr_q(gl);

    // copy the unchanged parts of the structure
    memcpy(gl->path, i->path, MAX_PATH_LENGTH);
    gl->w = i->w;
    gl->h = i->h;
    gl->s_w = i->s_w;
    gl->s_h = i->s_h;
    gl->cpu_time = i->cpu_time;

    // create the texture IDs
    glGenTextures(gl->s_w * gl->s_h, (GLuint*) &(gl->slices));
    for (int t = 0; t < gl->s_w * gl->s_h; t++) {
        glBindTexture(GL_TEXTURE_2D, gl->slices[t]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        GLint target_internal_format;
        GLint source_format;
        switch ( i->pf ) {
            case CPUIMAGE_GRAY:
                target_internal_format = GL_RED;
                source_format = GL_LUMINANCE;
                break;

            case CPUIMAGE_RGB:
                target_internal_format = GL_RGB;
                source_format = GL_RGB;
                break;

            case CPUIMAGE_BGR:
                target_internal_format = GL_RGB;
                source_format = GL_BGR;
                break;

            case CPUIMAGE_RGBA:
                target_internal_format = GL_RGBA;
                source_format = GL_RGBA;
                break;

            case CPUIMAGE_BGRA:
                target_internal_format = GL_RGBA;
                source_format = GL_BGRA;
                break;

            default:
                errx(1, "Unknown internal cpuimage pixel format %d", i->pf);
        }

#ifdef USE_MIPMAPS
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // trilinear interpolation
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gluBuild2DMipmaps(GL_TEXTURE_2D, target_internal_format, IMAGE_SLICE_SIZE, IMAGE_SLICE_SIZE, source_format, GL_UNSIGNED_BYTE, i->slices + 4*IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE*t);
#else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, target_internal_format, IMAGE_SLICE_SIZE, IMAGE_SLICE_SIZE, 0, source_format, GL_UNSIGNED_BYTE, i->slices + 4*IMAGE_SLICE_SIZE*IMAGE_SLICE_SIZE*t);
#endif

        // TODO: check glGetError
    }

    gl->gl_time = SDL_GetTicks() - start;

    return gl;
}

void glimage_draw(struct glimage *gl, float cx, float cy, float width, float height, float pixel_size, bool multidraw) {
    float x1 = cx - width/2;
    float x2 = cx + width/2;
    float y1 = cy - height/2;
    float y2 = cy + height/2;

    int slice = 0;
    float x_step = (x2-x1)/(gl->s_w - ((float) gl->s_w*IMAGE_SLICE_SIZE - gl->w)/IMAGE_SLICE_SIZE);
    float y_step = (y2-y1)/(gl->s_h - ((float) gl->s_h*IMAGE_SLICE_SIZE - gl->h)/IMAGE_SLICE_SIZE);

    for (int sy = 0; sy < gl->s_h; sy++) {
        double y_partial = 1;
        if ( sy == gl->s_h-1 )
            y_partial = ((double) gl->h - ((double) gl->s_h-1)*IMAGE_SLICE_SIZE)/IMAGE_SLICE_SIZE;

        double b = y1 + y_step* sy;
        double t = y1 + y_step*(sy+y_partial);

        for (int sx = 0; sx < gl->s_w; sx++) {
            double x_partial = 1;
            if ( sx == gl->s_w-1 )
                x_partial = ((double) gl->w - ((double) gl->s_w-1)*IMAGE_SLICE_SIZE)/IMAGE_SLICE_SIZE;

            double l = x1 + x_step* sx;
            double r = x1 + x_step*(sx+x_partial);

            glBindTexture(GL_TEXTURE_2D, gl->slices[slice]);

            if ( multidraw && (width/pixel_size < gl->w || height/pixel_size < gl->h) ) {
#ifndef MULTIDRAW_SIZE
#define MULTIDRAW_SIZE 2
#endif

#ifndef MULTIDRAW_SHARPNESS_FACTOR
#define MULTIDRAW_SHARPNESS_FACTOR 0.75
#endif

                float md_factor = (1.0/MULTIDRAW_SIZE)*MULTIDRAW_SHARPNESS_FACTOR*pixel_size;

                // this code does incorrect things with alpha-blended source images
                int drawn = 0;
                for (int dx = 0; dx <= MULTIDRAW_SIZE; dx++)
                    for (int dy = 0; dy <= MULTIDRAW_SIZE; dy++)
                        if ( dx != 0 || dy != 0 ) {
                            glColor4f(1,1,1,1.0/(drawn+1));

                            glBegin(GL_QUADS);
                            glTexCoord2f(0,0);
                            glVertex2f(l + dx*md_factor, b + dy*md_factor);
                            glTexCoord2f(x_partial,0);
                            glVertex2f(r + dx*md_factor, b + dy*md_factor);
                            glTexCoord2f(x_partial,y_partial);
                            glVertex2f(r + dx*md_factor, t + dy*md_factor);
                            glTexCoord2f(0,y_partial);
                            glVertex2f(l + dx*md_factor, t + dy*md_factor);
                            glEnd();

                            drawn++;
                        }
            } else {
                // CCW from bottom left
                glColor3f(1,1,1);
                glBegin(GL_QUADS);
                glTexCoord2f(0,0);
                glVertex2f(l,b);
                glTexCoord2f(x_partial,0);
                glVertex2f(r,b);
                glTexCoord2f(x_partial,y_partial);
                glVertex2f(r,t);
                glTexCoord2f(0,y_partial);
                glVertex2f(l,t);
                glEnd();
            }

#if DEBUG_DRAW_SLICES
            glColor3f(1,0.2,0.2);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBegin(GL_LINE_LOOP);
            glVertex2f(l,b);
            glVertex2f(r,b);
            glVertex2f(r,t);
            glVertex2f(l,t);
            glEnd();
#endif

            slice++;
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

static void glimage_free(struct glimage *gl) {
    glDeleteTextures(gl->s_w * gl->s_h, (GLuint*) &(gl->slices));
    free(gl);
}

void glimage_incr(void *gl) {
    ((struct glimage *)gl)->refcount++;
}

void glimage_decr(void *gl) {
    if ( !( --((struct glimage *) gl)->refcount ) )
        glimage_free((struct glimage *) gl);
}

