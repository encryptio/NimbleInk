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

static void zipper_replace_cpuimage(struct zipper *z, struct cpuimage *cpu) {
    if ( z->gl ) {
        glimage_decr_q(z->gl);
        z->gl = NULL;
    }

    if ( (z->gl = glimage_from_cpuimage(cpu)) == NULL )
        errx(1, "Couldn't put image '%s' from archive '%s' into OpenGL", z->ar.ar->names[z->ar.ar->map[z->ar.pos]], z->path);
    glimage_incr(z->gl);
}

static bool zipper_load_archive_image(struct zipper *z) {
    struct cpuimage *cpu;
    if ( (cpu = cpuimage_from_ram(z->ar.ar->data[ z->ar.ar->map[z->ar.pos]],
                                  z->ar.ar->sizes[z->ar.ar->map[z->ar.pos]])) == NULL ) {
        warnx("Couldn't load image '%s' from archive '%s'", z->ar.ar->names[z->ar.ar->map[z->ar.pos]], z->path);
        return false;
    }

    zipper_replace_cpuimage(z, cpu);

    return true;
}

static bool zipper_load_direct_image(struct zipper *z) {
    struct cpuimage *cpu;
    if ( (cpu = cpuimage_from_disk(z->path)) == NULL ) {
        warnx("Couldn't load image '%s'", z->path);
        return false;
    }

    zipper_replace_cpuimage(z, cpu);

    return true;
}

static bool zipper_prepare_new_archive(struct zipper *z, bool forwards) {
    if ( z->ar.ar ) {
        archive_decr_q(z->ar.ar);
        z->ar.ar = NULL;
    }

    if ( (z->ar.ar = archive_create(z->path)) == NULL ) {
        warnx("Couldn't prepare %s as an archive", z->path);
        return false;
    }

    if ( !archive_load_all(z->ar.ar) ) {
        warnx("Couldn't load all data from archive %s", z->path);
        z->ar.ar = NULL;
        return false;
    }

    z->ar.is = true;

    archive_incr(z->ar.ar);

    z->ar.pos = forwards ? 0 : z->ar.ar->files-1;

    return true;
}

static bool zipper_prepare_new_file(struct zipper *z, bool forwards) {
    if ( z->ar.ar ) {
        archive_decr_q(z->ar.ar);
        z->ar.ar = NULL;
    }

    if ( ft_file_is_archive(z->path) ) {
        if ( !zipper_prepare_new_archive(z, forwards) )
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

static void zipper_dir_down_check(struct zipper *z, bool forwards) {
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

            if ( !found || (forwards ? english_compare_natural(dp->d_name, first) < 0 : english_compare_natural(dp->d_name, first) > 0) ) {
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
            return zipper_dir_down_check(z, forwards);
        }
    }
}

//////////////////////////////////////////////////////////////////////

struct zipper * zipper_create(char *path) {
    struct zipper *z;
    if ( (z = calloc(1, sizeof(struct zipper))) == NULL )
        err(1, "Couldn't malloc space for zipper");
    z->refcount = 1;
    zipper_decr_q(z);

    z->updepth = 0;

    strncpy(z->path, path, MAX_PATH_LENGTH);
    z->path[MAX_PATH_LENGTH-1] = '\0';

    if ( z->path[strlen(z->path)-1] == '/' ) {
        z->updepth--;

        if ( !zipper_is_dir(z->path) ) {
            warnx("zipper_create called with trailing slash, but item was not a directory");
            return NULL;
        }
    }

    zipper_dir_down_check(z, true);

    if ( !zipper_prepare_new_file(z, true) )
        errx(1, "fixme 54982192");

    return z;
}

bool zipper_next(struct zipper *z) {
    // special case: move forward in an archive
    if ( z->ar.ar ) {
        if ( z->ar.pos+1 < z->ar.ar->files ) {
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

    zipper_dir_down_check(z, true);

    if ( !ft_file_is_image(z->path) && !ft_file_is_archive(z->path) )
        // not a known filetype
        return zipper_next(z);

    if ( !zipper_prepare_new_file(z, true) )
        // failed to load this file, try the next
        // TODO: goto?
        return zipper_next(z);

    return true;
}

bool zipper_prev(struct zipper *z) {
    // special case: move backward in an archive
    if ( z->ar.ar ) {
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

    zipper_dir_down_check(z, false);

    if ( !ft_file_is_image(z->path) && !ft_file_is_archive(z->path) )
        // not a known filetype
        return zipper_prev(z);

    if ( !zipper_prepare_new_file(z, false) )
        // failed to load this file, try the previous
        // TODO: goto?
        return zipper_prev(z);

    return true;
}

static void zipper_free(struct zipper *z) {
    errx(1, "zipper_free not implemented");
}

void zipper_incr(void *z) {
    ((struct zipper *)z)->refcount++;
}

void zipper_decr(void *z) {
    if ( !( --((struct zipper *) z)->refcount ) )
        zipper_free(z);
}

