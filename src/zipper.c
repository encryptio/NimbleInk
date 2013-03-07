#include "zipper.h"

#include "image.h"
#include "english.h"
#include "stringutils.h"
#include "filetype.h"
#include "inklog.h"
#define INKLOG_MODULE "zipper"

#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <assert.h>
#include <libgen.h>
#include <sys/stat.h>
#include <dirent.h>

static bool zipper_is_dir(char *path) {
    struct stat st;
    if ( stat(path, &st) )
        return false;

    return (st.st_mode & S_IFDIR) ? true : false;
}

static void * zipper_pos_load_archive_file(struct zipper_pos *p) {
    int mpos = p->ar.ar->map[p->ar.pos];

    uint8_t *data = malloc(p->ar.ar->sizes[mpos]);
    if ( data == NULL ) {
        inklog(LOG_ERR, "Couldn't malloc space for data file (%llu bytes)", (long long unsigned) p->ar.ar->sizes[mpos]);
        return NULL;
    }

    if ( !p->ar.ar->load(p->ar.ar, mpos, data) ) {
        warnx("Couldn't load file from archive (%s, index %d, interior filename %s)", p->path, mpos, p->ar.ar->names[mpos]);
        free(data);
        return NULL;
    }

    return data;
}

static bool zipper_pos_load_cpuimage(struct zipper_pos *p) {
    if ( p->cpu )
        return true;

    if ( p->ar.is ) {
        int mpos = p->ar.ar->map[p->ar.pos];

        void *data = zipper_pos_load_archive_file(p);
        if ( !data )
            return false;

        p->cpu = cpuimage_from_ram(data, p->ar.ar->sizes[mpos]);
        free(data);
        if ( !p->cpu )
            return false;
        p->cpu->incr(p->cpu);

        return true;
    } else {
        p->cpu = cpuimage_from_disk(p->path);
        if ( !p->cpu )
            return false;
        p->cpu->incr(p->cpu);

        return true;
    }
}

static bool zipper_pos_load_glimage(struct zipper_pos *p) {
    if ( p->gl )
        return true;

    if ( !zipper_pos_load_cpuimage(p) )
        return false;

    p->gl = glimage_from_cpuimage(p->cpu);
    if ( !p->gl )
        return false;
    glimage_incr(p->gl);

    return true;
}

static void zipper_pos_free(struct zipper_pos *p) {
    if ( p->cpu ) {
        p->cpu->decr(p->cpu);
        p->cpu = NULL;
    }
    if ( p->gl ) {
        glimage_decr_q(p->gl);
        p->gl = NULL;
    }
    if ( p->ar.ar ) {
        p->ar.ar->decr(p->ar.ar);
        p->ar.ar = NULL;
    }
}

// all string buffers assumed to be MAX_PATH_LENGTH bytes long
// TODO: remember sorted directory contents and stat() to check if a reload is needed
// TODO: make O(1) on repeated calls
static bool zipper_searchdir(char *out, char *dir, char *before, char *after, comparator_fn fn, bool first) {
    DIR *dh = opendir(dir);
    if ( !dh )
        return false;

    bool found = false;
    struct dirent *dp;
    while ( (dp = readdir(dh)) != NULL ) {

        if ( dp->d_name[0] == '.' )
            continue;

        if ( (!found || (first ? fn(dp->d_name, out) < 0 : fn(dp->d_name, out) > 0))
                && (!before || fn(dp->d_name, before) < 0)
                && (!after  || fn(dp->d_name, after)  > 0) ) {

            strncpy(out, dp->d_name, MAX_PATH_LENGTH);
            out[MAX_PATH_LENGTH-1] = '\0';
            found = true;
        }
    }

    closedir(dh);

    return found;
}

static void zipper_pos_downifdir(struct zipper_pos *p, bool forwards) {
    while ( zipper_is_dir(p->path) ) {
        char name[MAX_PATH_LENGTH];
        if ( !zipper_searchdir(name, p->path, NULL, NULL, english_compare_natural, forwards) )
            return;

        str_append(p->path, "/", MAX_PATH_LENGTH);
        str_append(p->path, name, MAX_PATH_LENGTH);

        p->updepth++;
    }
}

static bool zipper_pos_file_is_usable(struct zipper_pos *p) {
    if ( ft_path_matches_type(p->path, FT_IMAGE) || ft_path_matches_type(p->path, FT_ARCHIVE) )
        return true;

    return false;
}

static bool zipper_pos_archive_file_is_usable(struct zipper_pos *p) {
    assert(p->ar.is);

    int mpos = p->ar.ar->map[p->ar.pos];

    void *data = zipper_pos_load_archive_file(p);
    if ( !data )
        return false;

    int len = p->ar.ar->sizes[mpos];

    if ( len < FILETYPE_MAGIC_BYTES ) {
        free(data);
        return false;
    }

    bool ret = ft_magic_matches_type(data, FT_IMAGE);
    free(data);

    return ret;
}

static bool zipper_pos_prepare_if_archive(struct zipper_pos *p, bool forwards) {
    if ( !ft_path_matches_type(p->path, FT_ARCHIVE) )
        return true;

    p->ar.ar = archive_create(p->path);
    if ( !p->ar.ar )
        return false;
    p->ar.ar->incr(p->ar.ar);

    p->ar.is = true;
    p->ar.pos = forwards ? 0 : p->ar.ar->files-1;

    return true;
}

static bool zipper_pos_seek_inner(struct zipper_pos *p, bool forwards) {
    char name[MAX_PATH_LENGTH],
         aux_basename[MAX_PATH_LENGTH],
         aux_dirname[MAX_PATH_LENGTH],
         glibc_bullshit[MAX_PATH_LENGTH];

START:
    if ( p->ar.is ) {
        assert(p->ar.ar);

        // seek in this archive
        if ( forwards ) {
            while ( p->ar.pos < p->ar.ar->files-1 ) {
                p->ar.pos++;
                if ( zipper_pos_archive_file_is_usable(p) )
                    return true;
            }
        } else {
            while ( p->ar.pos > 0 ) {
                p->ar.pos--;
                if ( zipper_pos_archive_file_is_usable(p) )
                    return true;
            }
        }

        // hit the end of the archive

        p->ar.is = false; // cleaned up in wrapper
    }

    strncpy(glibc_bullshit, p->path, MAX_PATH_LENGTH);
    glibc_bullshit[MAX_PATH_LENGTH-1] = '\0';
    strncpy(aux_basename, basename(glibc_bullshit), MAX_PATH_LENGTH);
    aux_basename[MAX_PATH_LENGTH-1] = '\0';

    strncpy(glibc_bullshit, p->path, MAX_PATH_LENGTH);
    glibc_bullshit[MAX_PATH_LENGTH-1] = '\0';
    strncpy(aux_dirname, dirname(glibc_bullshit), MAX_PATH_LENGTH);
    aux_dirname[MAX_PATH_LENGTH-1] = '\0';

    if ( !zipper_searchdir(name, aux_dirname, forwards ? NULL : aux_basename, forwards ? aux_basename : NULL, english_compare_natural, forwards) ) {
        // couldn't find anything in this directory
        if ( p->updepth ) {
            // move up
            strncpy(glibc_bullshit, p->path, MAX_PATH_LENGTH);
            glibc_bullshit[MAX_PATH_LENGTH-1] = '\0';
            strncpy(p->path, dirname(glibc_bullshit), MAX_PATH_LENGTH);
            p->path[MAX_PATH_LENGTH-1] = '\0';

            p->updepth--;
            assert(p->updepth >= 0);

            goto START;
        } else {
            return false;
        }
    } else {
        // found a next/prev thing in this directory

        strncpy(p->path, aux_dirname, MAX_PATH_LENGTH);
        p->path[MAX_PATH_LENGTH-1] = '\0';

        str_append(p->path, "/", MAX_PATH_LENGTH);
        str_append(p->path, name, MAX_PATH_LENGTH);

        zipper_pos_downifdir(p, forwards);

        if ( !zipper_pos_file_is_usable(p) )
            goto START;

        if ( !zipper_pos_prepare_if_archive(p, forwards) )
            goto START;

        return true;
    }
}

static bool zipper_pos_seek(struct zipper_pos *p, bool forwards) {
    // handles the state change or lack of it,
    // while the search is done above.

    struct zipper_pos new;
    memcpy(&new, p, sizeof(struct zipper_pos));

    if ( !zipper_pos_seek_inner(&new, forwards) ) {
        // change nothing
        return false;
    }

    // seek succeeded, clean up any hanging references

    if ( p->cpu ) {
        p->cpu->decr(p->cpu);
        p->cpu = NULL;
        new.cpu = NULL;
    }
    if ( p->gl ) {
        glimage_decr_q(p->gl);
        p->gl = NULL;
        new.gl = NULL;
    }

    if ( p->ar.is != new.ar.is || p->ar.ar != new.ar.ar ) {
        if ( !new.ar.is ) {
            p->ar.is = false;
            new.ar.ar = NULL;
        }
        if ( p->ar.ar )
            p->ar.ar->decr(p->ar.ar);
    }

    memcpy(p, &new, sizeof(struct zipper_pos));

    return true;
}

static bool zipper_set_position(struct zipper *z, char *path, bool forwards) {
    for (int i = 0; i < z->pos_len; i++)
        zipper_pos_free(&(z->pos[i]));

    z->pos[0].updepth = 0;

    strncpy(z->pos[0].path, path, MAX_PATH_LENGTH-1);
    z->pos[0].path[MAX_PATH_LENGTH-1] = '\0';

    // if we set position to a path ending in a slash, it should be
    // a directory, and we should handle its contents only
    if ( z->pos[0].path[strlen(z->pos[0].path)-1] == '/' ) {
        if ( !zipper_is_dir(z->pos[0].path) ) {
            warnx("zipper_set_position called with a path with a trailing slash (%s), but item was not a directory", z->pos[0].path);
            return false;
        }

        z->pos[0].updepth--;
    }

    zipper_pos_downifdir(&(z->pos[0]), forwards);

    assert(z->pos[0].updepth >= 0);

    if ( !zipper_pos_file_is_usable(&(z->pos[0])) )
        if ( !zipper_pos_seek(&(z->pos[0]), true) )
            return false;

    if ( !zipper_pos_prepare_if_archive(&(z->pos[0]), forwards) )
        return false;

    z->pos_at = 0;
    z->pos_len = 1;

    return true;
}

struct zipper * zipper_create(char *path) {
    struct zipper *z;
    if ( (z = calloc(1, sizeof(struct zipper))) == NULL )
        err(1, "Couldn't allocate space for zipper");
    z->refcount = 1;
    zipper_decr_q(z);

    if ( !zipper_set_position(z, path, true) )
        return NULL;

    if ( z->pos[0].path[strlen(z->pos[0].path)-1] == '/' ) {
        z->pos[0].updepth--;

        if ( !zipper_is_dir(z->pos[0].path) ) {
            warnx("zipper_create called with trailing slash, but item was not a directory");
            return NULL;
        }
    }

    assert(z->pos[0].updepth >= 0);

    return z;
}

void zipper_clear_glimages(struct zipper *z) {
    for (int i = 0; i < z->pos_len; i++)
        if ( z->pos[i].gl ) {
            glimage_decr_q(z->pos[i].gl);
            z->pos[i].gl = NULL;
        }
}

struct glimage * zipper_current_glimage(struct zipper *z) {
    if ( !zipper_pos_load_glimage(&(z->pos[z->pos_at])) )
        return NULL;
    return z->pos[z->pos_at].gl;
}

static void zipper_make_pos_space(struct zipper *z, bool right_side) {
    // makes space and sets pos_len to the appropriate value

    if ( right_side ) {
        if ( z->pos_len + 1 < ZIPPER_MAX_NUM_POS ) {
            // enough space to just go ahead
            z->pos_len++;
        } else {
            // destroy left
            zipper_pos_free(&(z->pos[0]));
            z->pos_at--;
            memmove(z->pos, z->pos + 1, sizeof(struct zipper_pos)*(z->pos_len-1));
        }
    } else {
        if ( z->pos_len + 1 < ZIPPER_MAX_NUM_POS ) {
            // enough space to just shift over
            memmove(z->pos + 1, z->pos, sizeof(struct zipper_pos)*z->pos_len);
            z->pos_len++;
            z->pos_at++;
        } else {
            // destroy right
            zipper_pos_free(&(z->pos[z->pos_len-1]));
            memmove(z->pos + 1, z->pos, sizeof(struct zipper_pos)*(z->pos_len-1));
            z->pos_at++;
        }
    }

    assert(z->pos_at >= 0);
    assert(z->pos_at < z->pos_len);
}

static bool zipper_add_seek_pos(struct zipper *z, bool forwards) {
    struct zipper_pos new;
    memcpy(&new, &(z->pos[forwards ? z->pos_len-1 : 0]), sizeof(struct zipper_pos));
    new.cpu = NULL;
    new.gl = NULL;
    if ( new.ar.is )
        new.ar.ar->incr(new.ar.ar); // duplicated the pointer, and zipper_pos_seek assumes the refcounts are correct

    if ( zipper_pos_seek(&new, forwards) ) {
        zipper_make_pos_space(z, forwards);
        memcpy(&(z->pos[forwards ? z->pos_len-1 : 0]), &new, sizeof(struct zipper_pos));
        return true;
    } else {
        if ( new.ar.is )
            new.ar.ar->decr(new.ar.ar);
        return false;
    }
}

bool zipper_next(struct zipper *z) {
    if ( z->pos_at < z->pos_len-1 ) {
        z->pos_at++;
        return true;
    } else {
        if ( zipper_add_seek_pos(z, true) ) {
            z->pos_at++;
            return true;
        } else {
            return false;
        }
    }
}

bool zipper_prev(struct zipper *z) {
    if ( z->pos_at > 0 ) {
        z->pos_at--;
        return true;
    } else {
        if ( zipper_add_seek_pos(z, false) ) {
            z->pos_at--;
            return true;
        } else {
            return false;
        }
    }
}

bool zipper_tick_preload(struct zipper *z) {
    // check for image preloading opportunities
    for (int i = 0; i < z->pos_len; i++) {
        if ( !z->pos[i].cpu ) {
            return zipper_pos_load_cpuimage(&(z->pos[i]));
        }
        if ( !z->pos[i].gl ) {
            return zipper_pos_load_glimage(&(z->pos[i]));
        }
    }

    // check for zipper_pos preloading opportunities
    if ( ZIPPER_MAX_NUM_POS >= 3 ) {
        if ( z->pos_at == z->pos_len-1 )
            if ( zipper_add_seek_pos(z, true) ) {
                return true;
            }
        if ( z->pos_at == 0 )
            if ( zipper_add_seek_pos(z, false) ) {
                return true;
            }
    }

    return false;
}

static void zipper_free(struct zipper *z) {
    for (int i = 0; i < z->pos_len; i++)
        zipper_pos_free(&(z->pos[i]));
    free(z);
}

void zipper_incr(void *z) {
    ((struct zipper *)z)->refcount++;
}

void zipper_decr(void *z) {
    if ( !( --((struct zipper *) z)->refcount ) )
        zipper_free((struct zipper *) z);
}
