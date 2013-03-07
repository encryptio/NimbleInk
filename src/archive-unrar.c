#include "archive-unrar.h"
#include "stringutils.h"
#include "inklog.h"
#define INKLOG_MODULE "archive-unrar"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <ctype.h>

#define EXPECT(condition) if ( !(condition) ) { \
            inklog(LOG_ERR, "Condition failed in readline: %s", #condition); \
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
        strtol(c, &c, 10);
        while ( isspace(*c) ) c++;
        while ( isdigit(*c) ) c++; // percentage

        if ( *c != '%' ) {
            inklog(LOG_ERR, "Didn't get percentage in output of unrar");
            continue;
        }

        // ignore empty files
        if ( size == 0 )
            continue;

        for (i = 0; i < strlen(filename)+1; i++)
            ar->names[ar->files][i] = filename[i];
        ar->sizes[ar->files] = size;

        ar->files++;
    }

    return ar->files ? true : false;
}

#undef EXPECT

bool archive_load_single_rar(struct archive *ar, int which, uint8_t *into) {
    char cmd[1000];
    strcpy(cmd, "unrar p -kb -p- -cfg- -c- -idcpq ");
    str_append_quoted(cmd, ar->path, 1000);
    str_append(cmd, " ", 1000);
    str_append_quoted(cmd, ar->names[which], 1000);
    return archive_load_single_from_command(ar, cmd, which, into);
}
