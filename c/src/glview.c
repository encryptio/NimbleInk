#include "glview.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <stdlib.h>
#include <math.h>
#include <err.h>

#define GLVIEW_ROTATE_ANIM_LENGTH 0.2

#ifndef PI
#define PI 3.14159265
#endif

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

    gl->rot_anim_completeness = 1;

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

static void glview_image_size_rotation(struct glview *gl, float iw, float ih, float *dw, float *dh) {
    int floored = floor(gl->rot_at);
    float between = gl->rot_at - floored;

    bool floored_is_upright = floored % 2 == 0;

    float up_f = fit_factor(iw, ih, gl->w, gl->h);
    float right_f = fit_factor(ih, iw, gl->w, gl->h);

    float real_f;
    if ( floored_is_upright ) {
        real_f = right_f*between + up_f*(1-between);
    } else {
        real_f = up_f*between + right_f*(1-between);
    }

    *dw = real_f*iw;
    *dh = real_f*ih;
}

void glview_draw(struct glview *gl) {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    fprintf(stderr, "[glview] drawing frame %ld into %dx%d+%d,%d\n", gl->frameidx, viewport[2], viewport[3], viewport[0], viewport[1]);
    gl->frameidx++;

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
            float w,h;
            glview_image_size_rotation(gl, img->w, img->h, &w, &h);

            glPushMatrix();
            glTranslatef(gl->w/2, gl->h/2, 0);
            glRotatef(gl->rot_at*90, 0,0,1);
            glimage_draw(img, 0, 0, w, h, 1);
            glPopMatrix();
        } else {
            // TODO: draw a "broken image" image
            fprintf(stderr, "[glview] no image to draw\n");
        }
    } else {
        // TODO: draw a "no image" image
        fprintf(stderr, "[glview] no zipper to draw\n");
    }
}

bool glview_animate(struct glview *gl, float s) {
    fprintf(stderr, "[glview] animate %f seconds\n", s);
    bool ret = false;

    if ( gl->rot_anim_completeness < 1 ) {
        ret = true;
        gl->rot_anim_completeness += s/GLVIEW_ROTATE_ANIM_LENGTH;
        if ( gl->rot_anim_completeness >= 1 ) {
            gl->rot_at = gl->rot_anim_to;
            gl->rot_anim_completeness = 1;
        } else {
            float part = (gl->rot_anim_completeness + 1.0 - cosf(gl->rot_anim_completeness*PI))/3.0;
            gl->rot_at = gl->rot_anim_to * part + gl->rot_anim_from * (1-part);
        }
    }

    return ret;
}

void glview_set_rotate(struct glview *gl, int direction) {
    gl->rot_anim_from = gl->rot_at;
    gl->rot_anim_completeness = 0;
    gl->rot_anim_to += direction;
}

