#ifndef __GLVIEW_H__
#define __GLVIEW_H__

#include "zipper.h"

// usage notes:
// glview assumes you call glViewport yourself to set the draw region.
// it uses glGetIntegerv to retrieve this information at runtime.
// there is no reference counting with this object.

struct glview {
    int w,h;
    long frameidx;
    struct zipper *z;

    float rot_anim_to; // 0=upright, rotations are CCW in increments of 90 degrees
    float rot_anim_from;
    float rot_anim_completeness;
    float rot_at;

    bool multidraw;
};

struct glview *glview_create(void);
void glview_free(struct glview *gl);

void glview_set_zipper(struct glview *gl, struct zipper *z);
void glview_set_rotate(struct glview *gl, int direction);

void glview_draw(struct glview *gl);
bool glview_animate(struct glview *gl, float s);
bool glview_animating(struct glview *gl);

#endif
