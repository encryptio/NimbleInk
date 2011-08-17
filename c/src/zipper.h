#ifndef __ZIPPER_H__
#define __ZIPPER_H__

#include "archive.h"
#include "image.h"

#include <stdbool.h>

struct zipper {
    int updepth; // number of levels we're allowed to go up before reaching the "end"

    char path[MAX_PATH_LENGTH];

    bool image_initialized;
    struct glimage image;

    struct {
        bool is;
        bool initialized;
        struct archive ar;
        int map[ARCHIVE_MAX_FILES];
        int maplen;
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

void zipper_free(struct zipper *z);

#endif
