#ifndef __ZIPPER_H__
#define __ZIPPER_H__

#include "archive.h"
#include "image.h"
#include "reference.h"

#include <stdbool.h>

struct zipper {
    int refcount;

    int updepth; // number of levels we're allowed to go up before reaching the "end"

    char path[MAX_PATH_LENGTH];

    struct glimage *gl;

    struct {
        bool is;
        struct archive *ar;
        int pos;
    } ar;
};

struct zipper * zipper_create(char *path);

/* zipper_next semantics:
 * If the current file is an archive and we're not at the end of it:
 *      go to the next file in the archive
 * If the current file is a folder:
 *      go into it and increment updepth, restart
 * If the current file is the last in the folder:
 *      if updepth > 0:
 *          decrement updepth and go to the next file in the parent directory, restart
 *      otherwise:
 *          fail
 */
bool zipper_next(struct zipper *z);

/* zipper_prev emantics:
 * Reverse of zipper_next
 */
bool zipper_prev(struct zipper *z);

void zipper_incr(void *z);
void zipper_decr(void *z);

static inline void zipper_decr_q(struct zipper *z) {
    ref_queue_decr((void*)(z), zipper_decr);
}

#endif
