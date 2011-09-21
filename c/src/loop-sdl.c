#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <err.h>
#include <inttypes.h>

#include "zipper.h"
#include "glview.h"
#include "loop.h"

#define INITIAL_WIDTH 800
#define INITIAL_HEIGHT 550

struct loop_window {
    int refcount;
    SDL_Surface *screen;
    struct zipper *z;
    struct glview *gl;

    bool fullscreen;

    void *pt;
    loop_fn_want_immediate want_immediate;
    loop_fn_key_event keyevt;
};

bool running;
static int native_width;
static int native_height;
static struct loop_window *main_win = NULL;

extern void loop_setup_keypairs(void); // loop-common.c

static void reshape_gl(int w, int h) {
    // on some platforms, the gl context is destroyed when resizing, so we need to rebuild it from scratch
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(0,0,w,h);
}

void loop_window_set_title(struct loop_window *win, const char *name) {
    // woo, only one possible window. how stupid.
    SDL_WM_SetCaption(name, name);
}

struct zipper * loop_window_get_zipper(struct loop_window *win) {
    return win->z;
}

struct glview * loop_window_get_glview(struct loop_window *win) {
    return win->gl;
}

void loop_set_quit(void) {
    running = false;
}

void loop_initialize(void) {
    if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0 )
        errx(1, "Couldn't initialize SDL: %s", SDL_GetError());

    atexit(SDL_Quit);

    const SDL_VideoInfo *video_info = SDL_GetVideoInfo();

    native_width  = video_info->current_w;
    native_height = video_info->current_h;

    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    loop_setup_keypairs();
}

static int loop_map_keycode(int sdl_code) {
    if ( sdl_code < 256 ) return sdl_code; // matches on ASCII
    switch ( sdl_code ) {
        case SDLK_UP: return LKEY_UP;
        case SDLK_DOWN: return LKEY_DOWN;
        case SDLK_LEFT: return LKEY_LEFT;
        case SDLK_RIGHT: return LKEY_RIGHT;

        case SDLK_INSERT: return LKEY_INSERT;
        case SDLK_HOME: return LKEY_HOME;
        case SDLK_END: return LKEY_END;
        case SDLK_PAGEUP: return LKEY_PAGEUP;
        case SDLK_PAGEDOWN: return LKEY_PAGEDOWN;

        case SDLK_F1: return LKEY_F1;
        case SDLK_F2: return LKEY_F2;
        case SDLK_F3: return LKEY_F3;
        case SDLK_F4: return LKEY_F4;
        case SDLK_F5: return LKEY_F5;
        case SDLK_F6: return LKEY_F6;
        case SDLK_F7: return LKEY_F7;
        case SDLK_F8: return LKEY_F8;
        case SDLK_F9: return LKEY_F9;
        case SDLK_F10: return LKEY_F10;
        case SDLK_F11: return LKEY_F11;
        case SDLK_F12: return LKEY_F12;
        case SDLK_F13: return LKEY_F13;
        case SDLK_F14: return LKEY_F14;
        case SDLK_F15: return LKEY_F15;

        default: return -1;
    }
}

static void loop_post_keydown(int sdl_code) {
    if ( main_win && main_win->keyevt ) {
        int loop_code = loop_map_keycode(sdl_code);
        if ( loop_code <= 0 ) return;
        main_win->keyevt(main_win->pt, main_win, false, loop_code, 0); // TODO: modifier keys
    }
}

void loop_window_set_fullscreen(struct loop_window *win, bool fullscreen) {
    int target_width = fullscreen ? native_width : INITIAL_WIDTH;
    int target_height = fullscreen ? native_height : INITIAL_HEIGHT;
    win->fullscreen = fullscreen;
    win->screen = SDL_SetVideoMode(target_width, target_height, 32,
                                   SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE | (fullscreen ? SDL_FULLSCREEN : 0));
    if ( !win->screen )
        errx(1, "Couldn't set %dx%d video: %s", target_width, target_height, SDL_GetError());
    reshape_gl(win->screen->w, win->screen->h);
    zipper_clear_glimages(win->z);
    ref_release_pool(); // XXX UNSAFE
}

bool loop_window_get_fullscreen(struct loop_window *win) {
    return win->fullscreen;
}

void loop_until_quit(void) {
    assert(main_win);

    running = true;
    bool redraw_desired = true;
    bool loop_desired = false;
    bool animation_running = false;
    int last_ticks = 0;
    while ( running ) {
        SDL_Event evt;

        if ( main_win->want_immediate )
            redraw_desired = redraw_desired || main_win->want_immediate(main_win->pt, main_win);

        // TODO: SDL 1.3 adds SDL_WaitEventTimeout, which should be used here
        if ( (redraw_desired || loop_desired) ? SDL_PollEvent(&evt) : SDL_WaitEvent(&evt) ) {
            switch ( evt.type ) {
                case SDL_KEYDOWN:
                    loop_post_keydown( evt.key.keysym.sym );
                    redraw_desired = true;
                    break;

                case SDL_KEYUP:
                case SDL_MOUSEMOTION:
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                case SDL_JOYAXISMOTION:
                case SDL_JOYBALLMOTION:
                case SDL_JOYHATMOTION:
                case SDL_JOYBUTTONDOWN:
                case SDL_JOYBUTTONUP:
                case SDL_ACTIVEEVENT:
                    break;

                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_VIDEORESIZE:
                    main_win->screen = SDL_SetVideoMode( evt.resize.w, evt.resize.h, 32, SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE );
                    redraw_desired = true;
                    if ( !main_win->screen )
                        errx(1, "Couldn't set %dx%d video: %s", evt.resize.w, evt.resize.h, SDL_GetError());
                    reshape_gl(main_win->screen->w, main_win->screen->h);
                    zipper_clear_glimages(main_win->z);
                    ref_release_pool();
                    break;

                case SDL_SYSWMEVENT: // do we need to redraw here?
                case SDL_VIDEOEXPOSE:
                    redraw_desired = true;
                    break;

                default:
                    printf("unhandled event, type=%d\n", evt.type);
                    redraw_desired = true; // just in case
            }
        }

        loop_desired = false;

        int now_ticks = SDL_GetTicks();

        int ticks = now_ticks - last_ticks;
        last_ticks = now_ticks;
        if ( !animation_running )
            ticks = 0;

        if ( glview_animate(main_win->gl, ticks/1000.0) ) { // TODO: should this be before the draw?
            loop_desired = true;
            animation_running = true;
            redraw_desired = true;
        } else {
            animation_running = false;
        }

        if ( running && redraw_desired ) {
            // clear buffers
            glClearColor(0,0,0.1,0);
            glClear(GL_COLOR_BUFFER_BIT);

            glview_draw(main_win->gl);

            SDL_GL_SwapBuffers();

            redraw_desired = false;
        }

        if ( zipper_tick_preload(main_win->z) )
            loop_desired = true;

        ref_release_pool();
    }
}

struct loop_window *loop_window_open(
        char *path,
        void *pt,
        loop_fn_want_immediate want_immediate,
        loop_fn_key_event keyevt) {

    if ( main_win )
        errx(1, "Can't create more than one window with the SDL loop");

    struct loop_window *win;
    if ( (main_win = win = calloc(1, sizeof(struct loop_window))) == NULL )
        err(1, "Couldn't allocate space for loop_window");

    win->refcount = 1;
    win->pt = pt;
    win->want_immediate = want_immediate;
    win->keyevt = keyevt;

    win->screen = SDL_SetVideoMode( INITIAL_WIDTH, INITIAL_HEIGHT, 32, SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE );
    if ( !win->screen )
        errx(1, "Couldn't set %dx%d video: %s", INITIAL_WIDTH, INITIAL_HEIGHT, SDL_GetError());
    
    reshape_gl(win->screen->w, win->screen->h);

    win->z = zipper_create(path);
    if ( win->z == NULL )
        errx(1, "Couldn't create zipper\n");
    zipper_incr(win->z);

    win->gl = glview_create();
    glview_set_zipper(win->gl, win->z);

    loop_window_decr_q(win);
    return win;
}

static void loop_window_free(struct loop_window *win) {
    zipper_decr(win->z);
    glview_free(win->gl);
    free(win);

    // XXX: is main_win okay?
}

void loop_window_incr(void *win) {
    ((struct loop_window *) win)->refcount++;
}

void loop_window_decr(void *win) {
    if ( !( --((struct loop_window *) win)->refcount ) )
        loop_window_free((struct loop_window *) win);
}

