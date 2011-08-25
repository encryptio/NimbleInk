#include "english.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#define MAX_STRINGS 10000

int compare_strings(const void *l, const void *r) {
    return english_compare_natural(*((char**)l), *((char**)r));
}

int main(int argc, char **argv) {
    char *strings[MAX_STRINGS];
    int ct = 0;

    char this[1000];
    while ( fgets(this, 1000, stdin) ) {
        strings[ct++] = strdup(this);
        if ( ct == MAX_STRINGS )
            errx(1, "Too many strings");
    }

    qsort(strings, ct, sizeof(char*), compare_strings);

    for (int i = 0; i < ct; i++)
        if ( !fwrite(strings[i], strlen(strings[i]), 1, stdout) )
            errx(1, "Couldn't write out string %d", i);

    exit(0);
}

