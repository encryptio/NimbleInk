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

void reshape_gl(void) {
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

    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    SDL_Surface *screen = SDL_SetVideoMode( window_width, window_height, 32, SDL_OPENGL );
    if ( !screen )
        errx(1, "Couldn't set %dx%d video: %s", window_width, window_height, SDL_GetError());

    fprintf(stderr, "SDL Initialized\n");

    //////
    // Set up OpenGL
    
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    reshape_gl();

    glClearColor(0,0,1,0);
    glClear(GL_COLOR_BUFFER_BIT);

    fprintf(stderr, "GL Initialized\n");

    //////
    // Set up image
    
    struct zipper *z = zipper_create(argv[1]);

    do {
        printf("displaying image\n");
        printf("   z->path='%s'\n", z->path);
        printf("   z->ar.is=%d\n", z->ar.is ? 1 : 0);
        if ( z->ar.is )
            printf("   z->ar.ar.names[map[%d] = %d])='%s'\n", z->ar.pos, z->ar.map[z->ar.pos], z->ar.ar.names[z->ar.map[z->ar.pos]]);
        printf("image timings: load=%dms, cpu=%dms, gl=%dms\n", z->image.load_time, z->image.cpu_time, z->image.gl_time);
        
        printf("drawing\n");
        float f = fit_factor(z->image.w, z->image.h, window_width, window_height);
        float w = z->image.w*f;
        float h = z->image.h*f;
        glClear(GL_COLOR_BUFFER_BIT);
        image_draw(&(z->image), (window_width-w)/2, (window_height-h)/2, (window_width-w)/2+w, (window_height-h)/2+h);

        SDL_GL_SwapBuffers();
    } while ( zipper_next(z) );

    return 0;
}

