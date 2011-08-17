#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__

#include "path.h"

#include <stdbool.h>

#define ARCHIVE_MAX_FILES 5000

enum archive_type {
    archive_unknown = 0,
    archive_zip = 1,
    archive_rar = 2
};

struct archive {
    char path[MAX_PATH_LENGTH];
    enum archive_type type;

    // TODO: make dynamic
    char names[ARCHIVE_MAX_FILES][MAX_PATH_LENGTH];
    int sizes[ARCHIVE_MAX_FILES];
    void *data[ARCHIVE_MAX_FILES];

    int files;
    int files_loaded;
};

bool archive_prepare(char *path, struct archive *ar);
bool archive_load_all(struct archive *ar);
void archive_destroy(struct archive *ar);

#endif
