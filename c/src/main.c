#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "loop.h"
#include "reference.h"
#include "zipper.h"
#include "glview.h"

static void keyevt(void *pt, struct loop_window *win, bool is_repeat, uint32_t ch, uint8_t mods) {
    fprintf(stderr, "[main] keyevt ch=%d=%c\n", ch, ch);

    if ( ch == LKEY_RIGHT ) {
        zipper_next(loop_window_get_zipper(win));
        //set_window_name(z);
    } else if ( ch == LKEY_LEFT ) {
        zipper_prev(loop_window_get_zipper(win));
        //set_window_name(z);
    } else if ( ch == 'q' ) {
        loop_set_quit();
    } else if ( ch == 'm' ) {
        image_multidraw = !image_multidraw;
    } else if ( ch == 'f' ) {
        loop_window_set_fullscreen(win, !loop_window_get_fullscreen(win));
    } else if ( ch == 'r' ) {
        glview_set_rotate(loop_window_get_glview(win), (mods & LMOD_SHIFT) ? 1 : -1);
    }
}

int main(int argc, char **argv) {
    loop_initialize();

    struct loop_window **windows;
    if ( (windows = calloc(1, sizeof(struct loop_window *)*argc)) == NULL )
        err(1, "Couldn't allocate space for %d window pointers", argc);

    for (int i = 1; i < argc; i++) {
        printf("opening window for '%s'\n", argv[i]);
        windows[i] = loop_window_open(argv[i], NULL, NULL, keyevt);
        loop_window_incr(windows[i]);
    }
    
    loop_until_quit();

    ref_release_pool(); // last chance

    return 0;
}

