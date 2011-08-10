#include "archive.h"

#include "filetype.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <ctype.h>

static bool archive_load_toc(struct archive *ar);
static bool archive_load_toc_zip(struct archive *ar);
static bool archive_load_toc_rar(struct archive *ar);
static bool archive_load_all_zip(struct archive *ar);
static bool archive_load_all_rar(struct archive *ar);

//////////////////////////////////////////////////////////////////////

bool archive_prepare(char *path, struct archive *ar) {
    strncpy(ar->path, path, ARCHIVE_MAX_PATH_LENGTH);
    ar->path[ARCHIVE_MAX_PATH_LENGTH-1] = '\0';

    FILE *fh = fopen(ar->path, "rb");
    if ( fh == NULL )
        err(1, "Couldn't open %s for reading", ar->path);

    uint8_t magic_buf[8];
    if ( !fread(magic_buf, 8, 1, fh) )
        err(1, "Couldn't read from %s", ar->path);

    fclose(fh);

    if ( ft_is_rar(magic_buf) ) {
        ar->type = archive_rar;
    } else if ( ft_is_zip(magic_buf) ) {
        ar->type = archive_zip;
    } else {
        ar->type = archive_unknown;
        errx(1, "Couldn't determine type of archive in %s", ar->path);
    }

    ar->files = 0;
    ar->files_loaded = 0;

    return archive_load_toc(ar);
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

void archive_destroy(struct archive *ar) {
    for (int i = 0; i < ar->files; i++) {
        if ( ar->data[i] ) {
            ar->files_loaded--;
            free(ar->data[i]);
            ar->data[i] = NULL;
        }
    }
}

//////////////////////////////////////////////////////////////////////

static void make_command(char *prefix, char *filename, char *output, int n) {
    strncpy(output, prefix, n);
    output[n-1] = '\0';

    if ( strlen(output) < n-2 ) {
        output[strlen(output)+1] = '\0';
        output[strlen(output)  ] = '\'';
    } else {
        return;
    }

    int j = 0;
    for (int i = strlen(output); i < n-4;) {
        char ch = filename[j++];
        switch ( ch ) {
            case '\0':
                output[i++] = '\'';
                output[i++] = '\0';
                i = 1000;
                break;

            case '\'':
                output[i++] = '\'';
                output[i++] = '\\';
                output[i++] = '\'';
                output[i++] = '\'';
                break;

            default:
                output[i++] = ch;
                break;
        }
    }
}

//////////////////////////////////////////////////////////////////////

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

#define EXPECT(condition) if ( !(condition) ) { \
            warnx("Condition failed in readline: %s", #condition); \
            continue; \
        }

static bool archive_load_toc_zip(struct archive *ar) {
    char cmd[1000];
    make_command("unzip -l -qq ", ar->path, cmd, 1000);

    FILE *fh = popen(cmd, "r");
    if ( fh == NULL )
        err(1, "Couldn't popen command: %s", cmd);

    char line[1000];
    while ( fgets(line, 1000, fh) ) {
        // e.g.
        //    396385  2011-01-10 11:34   Yumekui_Merry_v01_p026.png

        char *c = line;

        while ( isspace(*c) ) c++;

        long length = strtol(c, &c, 10);

        while ( isspace(*c) ) c++;

        if ( length == 0 )
            continue; // silently

        EXPECT(length > 0);

        EXPECT(isdigit(*c++));
        EXPECT(isdigit(*c++));
        while ( isdigit(*c) ) c++; // some versions use 2-digit years, some 4-digit
        EXPECT(*c++ == '-');
        EXPECT(isdigit(*c++));
        EXPECT(isdigit(*c++));
        EXPECT(*c++ == '-');
        EXPECT(isdigit(*c++));
        EXPECT(isdigit(*c++));

        while ( isspace(*c) ) c++;

        while ( isdigit(*c) ) c++;
        EXPECT(*c++ == ':');
        while ( isdigit(*c) ) c++;

        while ( isspace(*c) ) c++;

        EXPECT(!isspace(*c) && *c != '\0');

        // all conditions satisfied, and we've reached the filename. time to read it into the directory.
        
        ar->sizes[ar->files] = length;
        int n = 0;
        while ( *c != '\0' && *c != '\n' ) {
            ar->names[ar->files][n++] = *c++;
            if ( n >= ARCHIVE_MAX_FILENAME_LENGTH-1 )
                break;
        }
        ar->names[ar->files][n] = '\0';
        ar->data[ar->files] = NULL;

        ar->files++;
    }

    pclose(fh);

    return ar->files ? true : false;
}

static bool archive_load_toc_rar(struct archive *ar) {
    char cmd[1000];
    make_command("unrar v -kb -p- -cfg- -c- ", ar->path, cmd, 1000);

    FILE *fh = popen(cmd, "r");
    if ( fh == NULL )
        err(1, "Couldn't popen command: %s", cmd);

    char line[1000];

    // skip until the starting -----...
    while ( fgets(line, 1000, fh) )
        if ( line[0] == '-' )
            break;

    char filename[ARCHIVE_MAX_FILENAME_LENGTH];
    while ( fgets(line, 1000, fh) ) {
        // end of listing
        if ( line[0] == '-' )
            break;

        // e.g.
        //  Transmetropolitan_14_p10.jpg
        //       428038   428038 100% 18-08-04 21:16  .....A.   4E9A34CD m0d 2.0

        char *c = line;
        while ( isspace(*c) ) c++;

        // filename on the first line, copy it out
        int i;
        for (i = 0; i < ARCHIVE_MAX_FILENAME_LENGTH-1; i++) {
            if ( *c == '\0' || *c == '\n' )
                break;

            filename[i] = *c++;
        }

        filename[i] = '\0';

        // next, the file statistics
        if ( !fgets(line, 1000, fh) )
            break;

        if ( line[0] == '-' )
            break;

        c = line;
        while ( isspace(*c) ) c++;
        long size = strtol(c, &c, 10);
        while ( isspace(*c) ) c++;
        long packed = strtol(c, &c, 10);
        while ( isspace(*c) ) c++;
        while ( isdigit(*c) ) c++; // percentage

        if ( *c != '%' ) {
            warnx("eek");
            continue;
        }

        if ( packed > size ) {
            warnx("eeek");
            continue;
        }

        // ignore empty files
        if ( size == 0 )
            continue;

        for (i = 0; i < strlen(filename)+1; i++)
            ar->names[ar->files][i] = filename[i];
        ar->sizes[ar->files] = size;
        ar->data[ar->files] = NULL;

        fprintf(stderr, "got file: %d bytes, filename \"%s\"\n", ar->sizes[ar->files], ar->names[ar->files]);

        ar->files++;
    }

    return ar->files ? true : false;
}

#undef EXPECT

//////////////////////////////////////////////////////////////////////

static void archive_load_all_from_filehandle(struct archive *ar, FILE *fh) {
    for (int i = 0; i < ar->files; i++) {
        printf("loading %d bytes for file %d, name \"%s\"\n", ar->sizes[i], i, ar->names[i]);

        void *new_data;
        if ( (new_data = malloc(ar->sizes[i])) == NULL )
            err(1, "Couldn't malloc space for file");

        if ( !fread(new_data, ar->sizes[i], 1, fh) )
            err(1, "Couldn't read file from archive pipe");

        // XXX memory fence, lock

        if ( ar->data[i] ) {
            free(ar->data[i]);
        } else {
            ar->files_loaded++;
        }
        ar->data[i] = new_data;
    }

    char extra;
    if ( fread(&extra, 1, 1, fh) )
        err(1, "Too much data in archive pipe");
}

static void archive_load_all_from_command(struct archive *ar, char *cmd) {
    FILE *fh = popen(cmd, "r");
    if ( fh == NULL )
        err(1, "Couldn't popen command: %s", cmd);

    archive_load_all_from_filehandle(ar, fh);

    pclose(fh);
}

static bool archive_load_all_zip(struct archive *ar) {
    char cmd[1000];
    make_command("unzip -p -qq ", ar->path, cmd, 1000);
    archive_load_all_from_command(ar, cmd);
    return true;
}

static bool archive_load_all_rar(struct archive *ar) {
    char cmd[1000];
    make_command("unrar p -kb -p- -cfg- -c- -idcdpq ", ar->path, cmd, 1000);
    archive_load_all_from_command(ar, cmd);
    return true;
}


