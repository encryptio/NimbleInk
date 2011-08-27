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
};

struct glview *glview_create(void);
void glview_free(struct glview *gl);

void glview_set_zipper(struct glview *gl, struct zipper *z);

void glview_draw(struct glview *gl);

#endif
