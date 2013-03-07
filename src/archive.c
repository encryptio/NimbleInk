#include "archive.h"
#include "inklog.h"
#define INKLOG_MODULE "archive"

#include "archive-unzip.h"
#include "archive-unrar.h"

#include "filetype.h"
#include "english.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <ctype.h>
#include <errno.h>

static bool archive_load_toc(struct archive *ar);
static void archive_make_map(struct archive *ar);
static void archive_incr(struct archive *ar);
static void archive_decr(struct archive *ar);
static void archive_decr_q(struct archive *ar);

//////////////////////////////////////////////////////////////////////

struct archive * archive_create(char *path) {
    struct archive *ar;
    if ( (ar = calloc(1, sizeof(struct archive))) == NULL ) {
        inklog(LOG_CRIT, "Couldn't allocate space for struct archive");
        return NULL;
    }

    ar->refcount = 1;
    ar->incr = archive_incr;
    ar->decr = archive_decr;
    ar->decr_q = archive_decr_q;

    ar->decr_q(ar);

    strncpy(ar->path, path, MAX_PATH_LENGTH);
    ar->path[MAX_PATH_LENGTH-1] = '\0';

    FILE *fh = fopen(ar->path, "rb");
    if ( fh == NULL ) {
        inklog(LOG_NOTICE, "Couldn't open %s for reading: %s", ar->path, strerror(errno));
        return NULL;
    }

    uint8_t magic_buf[FILETYPE_MAGIC_BYTES];
    if ( !fread(magic_buf, FILETYPE_MAGIC_BYTES, 1, fh) ) {
        inklog(LOG_NOTICE, "Couldn't read from %s: %s", ar->path, strerror(errno));
        fclose(fh);
        return NULL;
    }

    fclose(fh);

    if ( ft_magic_matches_type(magic_buf, FT_RAR) ) {
        ar->type = archive_rar;
        ar->load = archive_load_single_rar;
    } else if ( ft_magic_matches_type(magic_buf, FT_ZIP) ) {
        ar->type = archive_zip;
        ar->load = archive_load_single_zip;
    } else {
        ar->type = archive_unknown;
        inklog(LOG_WARNING, "Couldn't determine type of archive in %s", ar->path);
        return NULL;
    }

    if ( archive_load_toc(ar) ) {
        archive_make_map(ar);
        return ar;
    } else {
        return NULL;
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
        inklog(LOG_WARNING, "Couldn't read file %s:%d from archive pipe: %s", ar->path, which, strerror(errno));
        return false;
    }

    char extra;
    if ( fread(&extra, 1, 1, fh) ) {
        inklog(LOG_WARNING, "Too much data in archive pipe for file %s:%d", ar->path, which);
        return false;
    }

    return true;
}

bool archive_load_single_from_command(struct archive *ar, char *cmd, int which, uint8_t *into) {
    FILE *fh = popen(cmd, "r");
    if ( fh == NULL ) {
        inklog(LOG_WARNING, "Couldn't popen command %s: %s", cmd, strerror(errno));
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

static void archive_incr(struct archive *ar) {
    ar->refcount++;
}

static void archive_decr(struct archive *ar) {
    if ( !( --ar->refcount ) )
        archive_free(ar);
}

static void archive_decr_q(struct archive *ar) {
    ref_queue_decr((void*) ar, (decr_fn) ar->decr);
}
