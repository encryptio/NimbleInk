#include "zipper.h"

#include "filetype.h"
#include "archive.h"
#include "image.h"
#include "english.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __gnu_linux__
static int zipper_compare_files(const void *a, const void *b, void *thunk) {
#else
// OSX, *BSD
static int zipper_compare_files(void *thunk, const void *a, const void *b) {
#endif
    return english_compare_natural((((struct zipper *)thunk)->ar.ar.names[*((int*)a)]),
                                   (((struct zipper *)thunk)->ar.ar.names[*((int*)b)]));
}

static void zipper_replace_cpuimage(struct zipper *z, struct cpuimage *cpu) {
    if ( z->image_initialized ) {
        z->image_initialized = false;
        image_gl_destroy(&(z->image));
    }

    if ( !image_cpu2gl(cpu, &(z->image)) )
        errx(1, "Couldn't put image '%s' from archive '%s' into OpenGL", z->ar.ar.names[z->ar.map[z->ar.pos]], z->path);

    z->image_initialized = true;
}

static bool zipper_load_archive_image(struct zipper *z) {
    struct cpuimage cpu;
    if ( !image_load_from_ram(z->ar.ar.data[ z->ar.map[z->ar.pos]],
                             z->ar.ar.sizes[z->ar.map[z->ar.pos]], &cpu) ) {
        warnx("Couldn't load image '%s' from archive '%s'", z->ar.ar.names[z->ar.map[z->ar.pos]], z->path);
        return false;
    }

    zipper_replace_cpuimage(z, &cpu);

    image_cpu_destroy(&cpu);
    return true;
}

static bool zipper_load_direct_image(struct zipper *z) {
    struct cpuimage cpu;
    if ( !image_load_from_disk(z->path, &cpu) ) {
        warnx("Couldn't load image '%s'", z->path);
        return false;
    }

    zipper_replace_cpuimage(z, &cpu);

    image_cpu_destroy(&cpu);
    return true;
}

static bool zipper_prepare_new_archive(struct zipper *z) {
    if ( z->ar.initialized ) {
        z->ar.initialized = false;
        archive_destroy(&(z->ar.ar));
    }

    if ( !archive_prepare(z->path, &(z->ar.ar)) ) {
        warnx("Couldn't prepare %s as an archive", z->path);
        return false;
    }

    z->ar.initialized = true;

    if ( !archive_load_all(&(z->ar.ar)) ) {
        warnx("Couldn't load all data from archive %s", z->path);
        return false;
    }

    z->ar.is = true;
    z->ar.pos = 0;

    // ignore non-image files
    int j = 0;
    for (int i = 0; i < z->ar.ar.files; i++)
        if ( z->ar.ar.sizes[i] >= 8 && ft_is_image(z->ar.ar.data[i]) )
            z->ar.map[j++] = i;

    z->ar.maplen = j;

#ifdef __gnu_linux__
//extern void qsort_r (void *__base, size_t __nmemb, size_t __size,
//		     __compar_d_fn_t __compar, void *__arg)
//  __nonnull ((1, 4));
    qsort_r(&(z->ar.map), z->ar.maplen, sizeof(int), zipper_compare_files, (void*)z);
#else
    // OSX, *BSD
    qsort_r(&(z->ar.map), z->ar.maplen, sizeof(int), (void*)z, zipper_compare_files);
#endif

    return true;
}

static bool zipper_prepare_new_file(struct zipper *z) {
    z->ar.is = false;

    if ( z->ar.initialized ) {
        z->ar.initialized = false;
        archive_destroy(&(z->ar.ar));
    }

    if ( ft_file_is_archive(z->path) ) {
        if ( !zipper_prepare_new_archive(z) )
            return false;

        if ( !zipper_load_archive_image(z) )
            errx(1, "fixme 51398753");

        return true;

    } else if ( ft_file_is_image(z->path) ) {
        return zipper_load_direct_image(z);

    } else {
        errx(1, "zipper_prepare_new_file called when z->path (%s) is not an image or archive", z->path);
    }
}

static bool zipper_is_dir(char *path) {
    struct stat st;
    if ( stat(path, &st) )
        return false;

    return (st.st_mode & S_IFDIR) ? true : false;
}

static void zipper_dir_down_check(struct zipper *z) {
    if ( zipper_is_dir(z->path) ) {
        z->updepth++;

        DIR *dh = opendir(z->path);
        if ( !dh )
            return;

        char newpath[MAX_PATH_LENGTH];
        char first[MAX_PATH_LENGTH];
        bool found = false;
        struct dirent *dp;
        while ( (dp = readdir(dh)) != NULL ) {

            if ( dp->d_name[0] == '.' )
                continue;

            if ( !found || english_compare_natural(dp->d_name, first) < 0 ) {
                snprintf(newpath, MAX_PATH_LENGTH, "%s/%s", z->path, dp->d_name);
                if ( ft_file_is_image(newpath) || ft_file_is_archive(newpath) || zipper_is_dir(newpath) ) {
                    strncpy(first, dp->d_name, MAX_PATH_LENGTH);
                    first[MAX_PATH_LENGTH-1] = '\0';
                    found = true;
                }
            }
        }

        closedir(dh);

        if ( found ) {
            strncat(z->path, "/", MAX_PATH_LENGTH-strlen(z->path)-1);
            strncat(z->path, first, MAX_PATH_LENGTH-strlen(z->path)-1);

            // just in case we go down to another directory
            return zipper_dir_down_check(z);
        }
    }
}

//////////////////////////////////////////////////////////////////////

struct zipper * zipper_create(char *path) {
    struct zipper *z;
    if ( (z = malloc(sizeof(struct zipper))) == NULL )
        err(1, "Couldn't malloc space for zipper");

    z->updepth = 0;
    z->image_initialized = false;
    z->ar.initialized = false;

    strncpy(z->path, path, MAX_PATH_LENGTH);
    z->path[MAX_PATH_LENGTH-1] = '\0';

    if ( z->path[strlen(z->path)-1] == '/' ) {
        z->updepth--;

        if ( !zipper_is_dir(z->path) )
            errx(1, "zipper_create called with trailing slash, but item was not a directory");
    }

    zipper_dir_down_check(z);

    if ( !zipper_prepare_new_file(z) )
        errx(1, "fixme 54982192");

    return z;
}

bool zipper_next(struct zipper *z) {
    // special case: move forward in an archive
    if ( z->ar.is ) {
        if ( z->ar.pos+1 < z->ar.maplen ) {
            z->ar.pos++;
            zipper_load_archive_image(z); // ignore failures, let the display show a broken image
            return true;
        }
    }

    char pathdupe[MAX_PATH_LENGTH];

    strcpy(pathdupe, z->path);
    char *this_name = strdup(basename(pathdupe));

    strcpy(pathdupe, z->path);
    DIR *dh = opendir(dirname(pathdupe));
    if ( !dh ) {
        strcpy(pathdupe, z->path);
        warn("Couldn't opendir %s", dirname(pathdupe));
        return false;
    }

    char new_name[MAX_PATH_LENGTH];
    bool found_new = false;
    struct dirent *dp;
    while ( (dp = readdir(dh)) != NULL ) {

        if ( dp->d_name[0] == '.' )
            continue;


        if ( english_compare_natural(dp->d_name, this_name) > 0 && (!found_new || english_compare_natural(dp->d_name, new_name) < 0) ) {
            strncpy(new_name, dp->d_name, MAX_PATH_LENGTH);
            new_name[MAX_PATH_LENGTH-1] = '\0';
            found_new = true;
        }
    }

    closedir(dh);

    free(this_name);

    if ( !found_new ) {
        if ( z->updepth ) {
            z->updepth--;
            strcpy(pathdupe, z->path);
            strncpy(z->path, dirname(pathdupe), MAX_PATH_LENGTH);
            z->path[MAX_PATH_LENGTH-1] = '\0';
            return zipper_next(z);
        } else {
            return false;
        }
    }

    strcpy(pathdupe, z->path);
    snprintf(z->path, MAX_PATH_LENGTH, "%s/%s", dirname(pathdupe), new_name);

    zipper_dir_down_check(z);

    if ( !ft_file_is_image(z->path) && !ft_file_is_archive(z->path) ) {
        // not a known filetype
        return zipper_next(z);
    }

    if ( !zipper_prepare_new_file(z) ) {
        // failed to load this file, try the next
        // TODO: goto?
        return zipper_next(z);
    }

    return true;
}

bool zipper_prev(struct zipper *z) {
    // special case: move backward in an archive
    if ( z->ar.is ) {
        if ( z->ar.pos > 0 ) {
            z->ar.pos--;
            zipper_load_archive_image(z); // ignore failures, let the display show a broken image
            return true;
        }
    }

    char pathdupe[MAX_PATH_LENGTH];

    strcpy(pathdupe, z->path);
    char *this_name = strdup(basename(pathdupe));

    strcpy(pathdupe, z->path);
    DIR *dh = opendir(dirname(pathdupe));
    if ( !dh ) {
        strcpy(pathdupe, z->path);
        warn("Couldn't opendir %s", dirname(pathdupe));
        return false;
    }

    char new_name[MAX_PATH_LENGTH];
    bool found_new = false;
    struct dirent *dp;
    while ( (dp = readdir(dh)) != NULL ) {

        if ( dp->d_name[0] == '.' )
            continue;


        if ( english_compare_natural(dp->d_name, this_name) < 0 && (!found_new || english_compare_natural(dp->d_name, new_name) > 0) ) {
            strncpy(new_name, dp->d_name, MAX_PATH_LENGTH);
            new_name[MAX_PATH_LENGTH-1] = '\0';
            found_new = true;
        }
    }

    closedir(dh);

    free(this_name);

    if ( !found_new ) {
        if ( z->updepth ) {
            z->updepth--;
            strcpy(pathdupe, z->path);
            strncpy(z->path, dirname(pathdupe), MAX_PATH_LENGTH);
            z->path[MAX_PATH_LENGTH-1] = '\0';
            return zipper_prev(z);
        } else {
            return false;
        }
    }

    strcpy(pathdupe, z->path);
    snprintf(z->path, MAX_PATH_LENGTH, "%s/%s", dirname(pathdupe), new_name);

    zipper_dir_down_check(z);

    if ( !ft_file_is_image(z->path) && !ft_file_is_archive(z->path) ) {
        // not a known filetype
        return zipper_prev(z);
    }

    if ( !zipper_prepare_new_file(z) ) {
        // failed to load this file, try the previous
        // TODO: goto?
        return zipper_prev(z);
    }

    // go to the end of an archive if we went into one
    if ( z->ar.is ) {
        z->ar.pos = z->ar.maplen-1;
    }

    return true;
}

void zipper_free(struct zipper *z) {
    errx(1, "not implemented");
}

