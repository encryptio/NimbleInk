#include "glview.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <stdlib.h>
#include <err.h>

static float fit_factor(float mw, float mh, float w, float h) {
    float sf = w/mw;
    if ( sf*mh > h )
        sf = h/mh;
    return sf;
}

struct glview *glview_create(void) {
    struct glview *gl;
    if ( (gl = calloc(1, sizeof(struct glview))) == NULL )
        err(1, "Couldn't malloc space for glview");

    return gl;
}

void glview_free(struct glview *gl) {
    if ( gl->z )
        zipper_decr(gl->z);
    free(gl);
}

void glview_set_zipper(struct glview *gl, struct zipper *z) {
    if ( gl->z )
        zipper_decr(gl->z);
    gl->z = z;
    if ( gl->z )
        zipper_incr(gl->z);
}

void glview_draw(struct glview *gl) {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    fprintf(stderr, "[glview] drawing frame %ld into %dx%d+%d,%d\n", gl->frameidx, viewport[2], viewport[3], viewport[0], viewport[1]);

    gl->w = viewport[2];
    gl->h = viewport[3];

    // set camera: orthographic, pixel aligned, (0,0) in upper left
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,gl->w,gl->h,0,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if ( gl->z ) {
        struct glimage *img = zipper_current_glimage(gl->z);
        if ( img ) {
            float f = fit_factor(img->w, img->h, gl->w, gl->h);
            float w = img->w*f;
            float h = img->h*f;
            glimage_draw(img, (gl->w-w)/2, (gl->h-h)/2, (gl->w-w)/2+w, (gl->h-h)/2+h);
        } else {
            // TODO: draw a "broken image" image
            fprintf(stderr, "[glview] no image to draw\n");
        }
    } else {
        // TODO: draw a "no image" image
        fprintf(stderr, "[glview] no zipper to draw\n");
    }
}
