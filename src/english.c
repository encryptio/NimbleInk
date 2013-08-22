#include "english.h"

#include <ctype.h>
#include <stdlib.h>

int english_compare_natural(char *a, char *b) {
    // divide into parts, entirely numeric and entirely alpha.
    // then compare partwise. when the two parts are both numeric, convert
    // to integers and compare that way; otherwise, compare
    // alphanumerically, case-insensitive.
    //
    // this is equivalent to stepping and comparing each character, and
    // switching to a numeric only mode when both turn to numbers at the
    // same time.
    //
    // TODO: consider unicode.
    // TODO: consider skipping spaces and underscores

    char *oa = a;
    char *ob = b;

    while ( *a && *b ) {
        if ( isdigit(*a) && isdigit(*b) ) {
            // some digits, compare them numerically
            // XXX: is this safe from infinite loops?
            long long num_a = strtoll(a, &a, 10);
            long long num_b = strtoll(b, &b, 10);
            if ( num_a > num_b )
                return 1;
            else if ( num_a < num_b )
                return -1;
        } else {
            int l_a = tolower(*a++);
            int l_b = tolower(*b++);
            if ( l_a > l_b )
                return 1;
            else if ( l_a < l_b )
                return -1;
        }
    }

    // reached the end of at least one of the strings.
    // the shorter one compares lesser.
    if ( *a )
        return -1;
    if ( *b )
        return 1;

    // end of both, equal under case-insensitivity.

    // start over, case sensitive.
    a = oa; b = ob;
    while ( *a && *b ) {
        if ( *a > *b )
            return 1;
        else if ( *a < *b )
            return -1;
        a++; b++;
    }

    return 0;
}
