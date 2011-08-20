#include "archive.h"

#include "archive-unzip.h"
#include "archive-unrar.h"

#include "filetype.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <ctype.h>

static bool archive_load_toc(struct archive *ar);

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

    if ( ft_is_rar(magic_buf) ) {
        ar->type = archive_rar;
    } else if ( ft_is_zip(magic_buf) ) {
        ar->type = archive_zip;
    } else {
        ar->type = archive_unknown;
        warnx("Couldn't determine type of archive in %s", ar->path);
        return false;
    }

    if ( archive_load_toc(ar) )
        return ar;
    else
        return NULL;
}

bool archive_load_all(struct archive *ar) {
    switch ( ar->type ) {
        case archive_zip:
            return archive_load_all_zip(ar);
        case archive_rar:
            return archive_load_all_rar(ar);
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

bool archive_load_all_from_filehandle(struct archive *ar, FILE *fh) {
    for (int i = 0; i < ar->files; i++) {
        printf("loading %d bytes for file %d, name \"%s\"\n", ar->sizes[i], i, ar->names[i]);

        void *new_data;
        if ( (new_data = malloc(ar->sizes[i])) == NULL )
            err(1, "Couldn't malloc space for file");

        if ( !fread(new_data, ar->sizes[i], 1, fh) ) {
            warn("Couldn't read file from archive pipe");
            return false;
        }

        // XXX memory fence, lock

        if ( ar->data[i] ) {
            free(ar->data[i]);
        } else {
            ar->files_loaded++;
        }
        ar->data[i] = new_data;
    }

    char extra;
    if ( fread(&extra, 1, 1, fh) ) {
        warnx("Too much data in archive pipe");
        return false;
    }

    return true;
}

bool archive_load_all_from_command(struct archive *ar, char *cmd) {
    FILE *fh = popen(cmd, "r");
    if ( fh == NULL ) {
        warn("Couldn't popen command: %s", cmd);
        return false;
    }

    bool ret = archive_load_all_from_filehandle(ar, fh);

    pclose(fh);

    return ret;
}

//////////////////////////////////////////////////////////////////////

static void archive_free(struct archive *ar) {
    for (int i = 0; i < ar->files; i++)
        if ( ar->data[i] ) {
            ar->files_loaded--;
            free(ar->data[i]);
            ar->data[i] = NULL;
        }

    free(ar);
}

void archive_incr(void *ar) {
    ((struct archive *)ar)->refcount++;
}

void archive_decr(void *ar) {
    if ( !( --((struct archive *) ar)->refcount ) )
        archive_free((struct archive *) ar);
}

