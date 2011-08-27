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

#define INITIAL_WIDTH 800
#define INITIAL_HEIGHT 550

int native_width;
int native_height;
bool fullscreen = false;

void reshape_gl(int w, int h) {
    // on some platforms, the gl context is destroyed when resizing, so we need to rebuild it from scratch
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(0,0,w,h);
}

void set_window_name(struct zipper *z) {
    char name[1000];
    struct zipper_pos *pos = &(z->pos[z->pos_at]);
    if ( pos->ar.is )
        snprintf(name, 1000, "%s: (%d/%d) %s", pos->path, pos->ar.pos+1, pos->ar.ar->files, pos->ar.ar->names[pos->ar.ar->map[pos->ar.pos]]);
    else {
        strncpy(name, pos->path, 1000);
        name[999] = '\0';
    }

    // sanitize (OSX yells otherwise)
    int mv = 0;
    int i;
    for (i = 0; name[i]; i++)
        if ( !isprint(name[i]) )
            mv++;
        else
            name[i-mv] = name[i];
    name[i-mv] = '\0';

    SDL_WM_SetCaption(name, name);
}

int main(int argc, char **argv) {
    //////
    // Set up SDL

    if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0 )
        errx(1, "Couldn't initialize SDL: %s", SDL_GetError());

    atexit(SDL_Quit);

    const SDL_VideoInfo *video_info = SDL_GetVideoInfo();

    native_width  = video_info->current_w;
    native_height = video_info->current_h;

    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    SDL_Surface *screen = SDL_SetVideoMode( INITIAL_WIDTH, INITIAL_HEIGHT, 32, SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE );
    if ( !screen )
        errx(1, "Couldn't set %dx%d video: %s", INITIAL_WIDTH, INITIAL_HEIGHT, SDL_GetError());

    //////
    // Set up OpenGL
    
    reshape_gl(screen->w, screen->h);

    //////
    // Set up image
    
    struct zipper *z = zipper_create(argv[1]);
    if ( z == NULL )
        errx(1, "Couldn't create zipper\n");
    zipper_incr(z);

    struct glview *gl = glview_create();
    glview_set_zipper(gl, z);

    set_window_name(z);

    bool running = true;
    bool do_draw = true;
    bool draw_immediately = true;
    while ( running ) {
        SDL_Event evt;
        do_draw = true;
        // TODO: SDL 1.3 adds SDL_WaitEventTimeout, which should be used here
        if ( draw_immediately ? SDL_PollEvent(&evt) : SDL_WaitEvent(&evt) ) {
            switch ( evt.type ) {
                case SDL_KEYDOWN:
                    if ( evt.key.keysym.sym == SDLK_RIGHT ) {
                        zipper_next(z);
                        set_window_name(z);
                    } else if ( evt.key.keysym.sym == SDLK_LEFT ) {
                        zipper_prev(z);
                        set_window_name(z);
                    } else if ( evt.key.keysym.sym == SDLK_q ) {
                        running = false;
                    } else if ( evt.key.keysym.sym == SDLK_m ) {
                        image_multidraw = !image_multidraw;
                    } else if ( evt.key.keysym.sym == SDLK_f ) {
                        if ( !fullscreen ) {
                            fullscreen = true;
                            screen = SDL_SetVideoMode(native_width, native_height, 32, SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE | SDL_FULLSCREEN);
                            if ( !screen )
                                errx(1, "Couldn't set %dx%d video: %s", native_width, native_height, SDL_GetError());
                            reshape_gl(screen->w, screen->h);
                        } else {
                            fullscreen = false;
                            screen = SDL_SetVideoMode(INITIAL_WIDTH, INITIAL_HEIGHT, 32, SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE);
                            if ( !screen )
                                errx(1, "Couldn't set %dx%d video: %s", INITIAL_WIDTH, INITIAL_HEIGHT, SDL_GetError());
                            reshape_gl(screen->w, screen->h);
                        }
                    }
                    break;

                case SDL_KEYUP:
                case SDL_MOUSEMOTION:
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    do_draw = false;
                    break;

                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_VIDEORESIZE:
                    screen = SDL_SetVideoMode( evt.resize.w, evt.resize.h, 32, SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE );
                    if ( !screen )
                        errx(1, "Couldn't set %dx%d video: %s", evt.resize.w, evt.resize.h, SDL_GetError());
                    reshape_gl(screen->w, screen->h);
                    break;

                case SDL_ACTIVEEVENT:
                default:
                    printf("unhandled event, type=%d\n", evt.type);
            }
        }

        if ( do_draw && running ) {
            printf("drawing at %d\n", SDL_GetTicks());

            // clear buffers
            glClearColor(0,0,0.1,0);
            glClear(GL_COLOR_BUFFER_BIT);

            glview_draw(gl);

            SDL_GL_SwapBuffers();
        }

        draw_immediately = true;
        for (int i = 0; i < 4 && draw_immediately; i++)
            draw_immediately = zipper_tick_preload(z);

        ref_release_pool();
    }

    glview_free(gl);
    zipper_decr_q(z);
    ref_release_pool();

    return 0;
}

