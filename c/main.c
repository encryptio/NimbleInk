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
    
    struct archive ar;
    archive_prepare(argv[1], &ar);
    archive_load_all(&ar);
    
    struct cpuimage cpu;
    struct glimage gl;

    for (int i = 0; i < ar.files; i++) {
        printf("image: %s\n", ar.names[i]);

        printf("loading cpuimage\n");
        image_load_from_ram(ar.data[i], ar.sizes[i], &cpu);
        printf("cpu2gl\n");
        image_cpu2gl(&cpu, &gl);
        printf("cpu destroy\n");
        image_cpu_destroy(&cpu);

        printf("timings: load=%dms, cpu=%dms, gl=%dms\n", gl.load_time, gl.cpu_time, gl.gl_time);
        
        printf("drawing\n");
        float f = fit_factor(gl.w, gl.h, window_width, window_height);
        float w = gl.w*f;
        float h = gl.h*f;
        glClear(GL_COLOR_BUFFER_BIT);
        image_draw(&gl, (window_width-w)/2, (window_height-h)/2, (window_width-w)/2+w, (window_height-h)/2+h);

        image_gl_destroy(&gl);

        SDL_GL_SwapBuffers();
    }

    return 0;
}

