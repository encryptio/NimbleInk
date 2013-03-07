#include "archive.h"

#include "archive-unzip.h"
#include "archive-unrar.h"

#include "filetype.h"
#include "english.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <ctype.h>

static bool archive_load_toc(struct archive *ar);
static void archive_make_map(struct archive *ar);

//////////////////////////////////////////////////////////////////////

struct archive * archive_create(char *path) {
    struct archive *ar;
    if ( (ar = calloc(1, sizeof(struct archive))) == NULL )
        err(1, "Couldn't allocate space for archive");
    ar->refcount = 1;
    archive_decr_q(ar);

    strncpy(ar->path, path, MAX_PATH_LENGTH);
    ar->path[MAX_PATH_LENGTH-1] = '\0';

    FILE *fh = fopen(ar->path, "rb");
    if ( fh == NULL ) {
        warn("Couldn't open %s for reading", ar->path);
        return false;
    }

    uint8_t magic_buf[FILETYPE_MAGIC_BYTES];
    if ( !fread(magic_buf, FILETYPE_MAGIC_BYTES, 1, fh) ) {
        warn("Couldn't read from %s", ar->path);
        return false;
    }

    fclose(fh);

    if ( ft_magic_matches_type(magic_buf, FT_RAR) ) {
        ar->type = archive_rar;
    } else if ( ft_magic_matches_type(magic_buf, FT_ZIP) ) {
        ar->type = archive_zip;
    } else {
        ar->type = archive_unknown;
        warnx("Couldn't determine type of archive in %s", ar->path);
        return false;
    }

    if ( archive_load_toc(ar) ) {
        archive_make_map(ar);
        return ar;
    } else {
        return NULL;
    }
}

bool archive_load_single(struct archive *ar, int which, uint8_t *into) {
    switch ( ar->type ) {
        case archive_zip:
            return archive_load_single_zip(ar, which, into);
        case archive_rar:
            return archive_load_single_rar(ar, which, into);
        default:
            errx(1, "Unknown archive type %d", ar->type);
    }
}

static bool archive_load_toc(struct archive *ar) {
    switch ( ar->type ) {
        case archive_zip:
            return archive_load_toc_zip(ar);
        case archive_rar:
            return archive_load_toc_rar(ar);
        default:
            errx(1, "Unknown archive type %d", ar->type);
    }
}

//////////////////////////////////////////////////////////////////////

bool archive_load_single_from_filehandle(struct archive *ar, FILE *fh, int which, uint8_t *into) {
    if ( !fread(into, ar->sizes[which], 1, fh) ) {
        warn("Couldn't read file from archive pipe");
        return false;
    }

    char extra;
    if ( fread(&extra, 1, 1, fh) ) {
        warnx("Too much data in archive pipe");
        return false;
    }

    return true;
}

bool archive_load_single_from_command(struct archive *ar, char *cmd, int which, uint8_t *into) {
    FILE *fh = popen(cmd, "r");
    if ( fh == NULL ) {
        warn("Couldn't popen command: %s", cmd);
        return false;
    }

    bool ret = archive_load_single_from_filehandle(ar, fh, which, into);

    pclose(fh);

    return ret;
}

#ifdef __gnu_linux__
static int archive_compare_files(const void *a, const void *b, void *thunk) {
#else
// OSX, *BSD
static int archive_compare_files(void *thunk, const void *a, const void *b) {
#endif
    return english_compare_natural((((struct archive *)thunk)->names[*((int*)a)]),
                                   (((struct archive *)thunk)->names[*((int*)b)]));
}

static void archive_make_map(struct archive *ar) {
    for (int i = 0; i < ar->files; i++)
        ar->map[i] = i;

#ifdef __gnu_linux__
    qsort_r(&(ar->map), ar->files, sizeof(int), archive_compare_files, (void*)ar);
#else
    // OSX, *BSD
    qsort_r(&(ar->map), ar->files, sizeof(int), (void*)ar, archive_compare_files);
#endif
}

//////////////////////////////////////////////////////////////////////

static void archive_free(struct archive *ar) {
    free(ar);
}

void archive_incr(void *ar) {
    ((struct archive *)ar)->refcount++;
}

void archive_decr(void *ar) {
    if ( !( --((struct archive *) ar)->refcount ) )
        archive_free((struct archive *) ar);
}
