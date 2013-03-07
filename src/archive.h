#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__

#include "path.h"
#include "reference.h"

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

#define ARCHIVE_MAX_FILES 5000

enum archive_type {
    archive_unknown = 0,
    archive_zip = 1,
    archive_rar = 2
};

struct archive {
    int refcount;
    bool (*load)(struct archive *self, int which, uint8_t *into);

    char path[MAX_PATH_LENGTH];
    enum archive_type type;

    // TODO: make dynamic
    char names[ARCHIVE_MAX_FILES][MAX_PATH_LENGTH];
    int64_t sizes[ARCHIVE_MAX_FILES];

    int map[ARCHIVE_MAX_FILES];

    int files;
    int files_loaded;
};

struct archive * archive_create(char *path);

void archive_incr(void *ar);
void archive_decr(void *ar);

static inline void archive_decr_q(struct archive *ar) {
    ref_queue_decr((void*)(ar), archive_decr);
}

// Private
bool archive_load_single_from_filehandle(struct archive *ar, FILE *fh, int which, uint8_t *into);
bool archive_load_single_from_command(struct archive *ar, char *cmd, int which, uint8_t *into);

#endif
