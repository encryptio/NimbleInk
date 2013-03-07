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
    void (*incr)(struct archive *self);
    void (*decr)(struct archive *self);
    void (*decr_q)(struct archive *self);

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

// Private
bool archive_load_single_from_filehandle(struct archive *ar, FILE *fh, int which, uint8_t *into);
bool archive_load_single_from_command(struct archive *ar, char *cmd, int which, uint8_t *into);

#endif
