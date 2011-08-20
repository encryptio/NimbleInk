#include "archive-unrar.h"
#include "stringutils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <ctype.h>

#define EXPECT(condition) if ( !(condition) ) { \
            warnx("Condition failed in readline: %s", #condition); \
            continue; \
        }

bool archive_load_toc_rar(struct archive *ar) {
    char cmd[1000];
    strcpy(cmd, "unrar v -kb -p- -cfg- -c- ");
    str_append_quoted(cmd, ar->path, 1000);

    FILE *fh = popen(cmd, "r");
    if ( fh == NULL )
        err(1, "Couldn't popen command: %s", cmd);

    char line[1000];

    // skip until the starting -----...
    while ( fgets(line, 1000, fh) )
        if ( line[0] == '-' )
            break;

    char filename[MAX_PATH_LENGTH];
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
        for (i = 0; i < MAX_PATH_LENGTH-1; i++) {
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

        // files can get bigger if compressed incorrectly; however, if the
        // difference is too big, we've probably made a mistake.
        if ( packed*0.9 > size ) {
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

bool archive_load_all_rar(struct archive *ar) {
    char cmd[1000];
    strcpy(cmd, "unrar p -kb -p- -cfg- -c- -idcpq ");
    str_append_quoted(cmd, ar->path, 1000);
    return archive_load_all_from_command(ar, cmd);
}


