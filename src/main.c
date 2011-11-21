#ifdef LOOP_SDL
#include <SDL.h>
#endif

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "loop.h"
#include "reference.h"
#include "zipper.h"
#include "glview.h"

static void update_window_name(struct loop_window *win) {
    struct zipper *z = loop_window_get_zipper(win);

    char name[1000];
    struct zipper_pos *pos = &(z->pos[z->pos_at]);
    if ( pos->ar.is )
        snprintf(name, 1000, "%s: (%d/%d) %s", pos->path, pos->ar.pos+1, pos->ar.ar->files, pos->ar.ar->names[pos->ar.ar->map[pos->ar.pos]]);
    else {
        strncpy(name, pos->path, 1000);
        name[999] = '\0';
    }

    // sanitize (OSX w/ SDL yells otherwise)
    int mv = 0;
    int i;
    for (i = 0; name[i]; i++)
        if ( !isprint(name[i]) )
            mv++;
        else
            name[i-mv] = name[i];
    name[i-mv] = '\0';

    loop_window_set_title(win, name);
}

static void keyevt(void *pt, struct loop_window *win, bool is_repeat, uint32_t ch, uint8_t mods) {
    if ( ch == LKEY_RIGHT ) {
        zipper_next(loop_window_get_zipper(win));
        update_window_name(win);
    } else if ( ch == LKEY_LEFT ) {
        zipper_prev(loop_window_get_zipper(win));
        update_window_name(win);
    } else if ( ch == 'q' ) {
        loop_set_quit();
    } else if ( ch == 'm' ) {
        struct glview *gl = loop_window_get_glview(win);
        gl->multidraw = !gl->multidraw;
    } else if ( ch == 'f' ) {
        loop_window_set_fullscreen(win, !loop_window_get_fullscreen(win));
    } else if ( ch == 'r' ) {
        glview_set_rotate(loop_window_get_glview(win), (mods & LMOD_SHIFT) ? 1 : -1);
    }
}

int main(int argc, char **argv) {
    loop_initialize(&argc, &argv);

    struct loop_window **windows;
    if ( (windows = calloc(1, sizeof(struct loop_window *)*argc)) == NULL )
        err(1, "Couldn't allocate space for %d window pointers", argc);

    for (int i = 1; i < argc; i++) {
        printf("opening window for '%s'\n", argv[i]);
        windows[i] = loop_window_open(argv[i], NULL, NULL, keyevt);
        loop_window_incr(windows[i]);
        update_window_name(windows[i]);
    }
    
    loop_until_quit();

    ref_release_pool(); // last chance
    free(windows);

    return 0;
}

