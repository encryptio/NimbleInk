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

#include "image.h"
#include "archive.h"
#include "zipper.h"

int window_width = 800;
int window_height = 550;
int native_width;
int native_height;
bool fullscreen = false;

void reshape_gl(void) {
    // on some platforms, the gl context is destroyed when resizing, so we need to rebuild it from scratch
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0,0,0.1,0);
    glClear(GL_COLOR_BUFFER_BIT);

    // now the actual resize parts
    glViewport(0,0,window_width,window_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,window_width,window_height,0,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

float fit_factor(float mw, float mh, float w, float h) {
    float sf = w/mw;
    if ( sf*mh > h )
        sf = h/mh;
    return sf;
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

    SDL_Surface *screen = SDL_SetVideoMode( window_width, window_height, 32, SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE );
    if ( !screen )
        errx(1, "Couldn't set %dx%d video: %s", window_width, window_height, SDL_GetError());

    //////
    // Set up OpenGL
    
    reshape_gl();

    //////
    // Set up image
    
    struct zipper *z = zipper_create(argv[1]);

    bool running = true;
    while ( running ) {
        SDL_Event evt;
        // TODO: SDL 1.3 adds SDL_WaitEventTimeout, which should be used here
        if ( SDL_WaitEvent(&evt) ) {
            switch ( evt.type ) {
                case SDL_KEYDOWN:
                    if ( evt.key.keysym.sym == SDLK_RIGHT ) {
                        zipper_next(z);
                    } else if ( evt.key.keysym.sym == SDLK_LEFT ) {
                        zipper_prev(z);
                    } else if ( evt.key.keysym.sym == SDLK_q ) {
                        running = false;
                    } else if ( evt.key.keysym.sym == SDLK_m ) {
                        image_multidraw = !image_multidraw;
                    } else if ( evt.key.keysym.sym == SDLK_f ) {
                        if ( !fullscreen ) {
                            fullscreen = true;
                            screen = SDL_SetVideoMode(native_width, native_height, 32, SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE | SDL_FULLSCREEN);
                            if ( !screen )
                                errx(1, "Couldn't set %dx%d video: %s", window_width, window_height, SDL_GetError());
                            window_width = screen->w;
                            window_height = screen->h;
                            reshape_gl();
                        } else {
                            fullscreen = false;
                            screen = SDL_SetVideoMode(800, 600, 32, SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE);
                            if ( !screen )
                                errx(1, "Couldn't set %dx%d video: %s", window_width, window_height, SDL_GetError());
                            window_width = screen->w;
                            window_height = screen->h;
                            reshape_gl();
                        }
                    }
                    break;

                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_VIDEORESIZE:
                    window_width  = evt.resize.w;
                    window_height = evt.resize.h;
                    screen = SDL_SetVideoMode( window_width, window_height, 32, SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE );
                    if ( !screen )
                        errx(1, "Couldn't set %dx%d video: %s", window_width, window_height, SDL_GetError());
                    reshape_gl();
                    break;

                default:
                    printf("unhandled event, type=%d\n", evt.type);
            }
        }

        printf("drawing at %d\n", SDL_GetTicks());
        float f = fit_factor(z->image.w, z->image.h, window_width, window_height);
        float w = z->image.w*f;
        float h = z->image.h*f;
        glClear(GL_COLOR_BUFFER_BIT);
        image_draw(&(z->image), (window_width-w)/2, (window_height-h)/2, (window_width-w)/2+w, (window_height-h)/2+h);
        SDL_GL_SwapBuffers();
    }

    return 0;
}

