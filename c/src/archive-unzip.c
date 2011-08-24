#include "archive-unzip.h"

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

bool archive_load_toc_zip(struct archive *ar) {
    char cmd[1000];
    strcpy(cmd, "unzip -l -qq ");
    str_append_quoted_as_unzip_literal(cmd, ar->path, 1000);

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
            if ( n >= MAX_PATH_LENGTH-1 )
                break;
        }
        ar->names[ar->files][n] = '\0';
        ar->data[ar->files] = NULL;

        ar->files++;
    }

    pclose(fh);

    return ar->files ? true : false;
}

//////////////////////////////////////////////////////////////////////

bool archive_load_all_zip(struct archive *ar) {
    char cmd[1000];
    strcpy(cmd, "unzip -p -qq ");
    str_append_quoted_as_unzip_literal(cmd, ar->path, 1000);
    return archive_load_all_from_command(ar, cmd);
}

bool archive_load_single_zip(struct archive *ar, int which) {
    char cmd[1000];
    strcpy(cmd, "unzip -p -qq ");
    str_append_quoted_as_unzip_literal(cmd, ar->path, 1000);
    str_append(cmd, " ", 1000);
    str_append_quoted_as_unzip_literal(cmd, ar->names[which], 1000);
    return archive_load_single_from_command(ar, cmd, which);
}

