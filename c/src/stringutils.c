#include "stringutils.h"

void str_append_quoted(char *dst, const char *src, size_t n) {
    int ct = 0;
    while ( ct < n && dst[ct] ) ct++; // seek to end of string

    // TODO: off by one?
    if ( ct+1 < n )
        dst[ct++] = '\'';

    n -= 1; // need an extra ' at the end

    while ( *src && ct < n ) {
        if ( *src == '\'' ) {
            if ( ct+4 < n ) {
                dst[ct++] = '\'';
                dst[ct++] = '\\';
                dst[ct++] = '\'';
                dst[ct++] = '\'';
                src++;
            } else {
                goto CLOSESTR;
            }
        } else {
            dst[ct++] = *src++;
        }
    }

CLOSESTR:
    dst[ct++] = '\'';
    dst[ct] = '\0';
}

void str_append(char *dst, const char *src, size_t n) {
    int ct = 0;
    while ( ct < n && dst[ct] ) ct++; // seek to end of string

    while ( *src && ct < n )
        dst[ct++] = *src++;

    dst[ct] = '\0';
}

