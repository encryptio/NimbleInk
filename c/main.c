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

int main(int argc, char **argv) {
    //////
    // Set up SDL

    if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0 )
        errx(1, "Couldn't initialize SDL: %s", SDL_GetError());

    atexit(SDL_Quit);

    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    SDL_Surface *screen = SDL_SetVideoMode( 640, 480, 32, SDL_OPENGL );
    if ( !screen )
        errx(1, "Couldn't set 640x480 video: %s", SDL_GetError());

    fprintf(stderr, "SDL Initialized\n");

    //////
    // Set up OpenGL
    
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0,0,1,0);
    glViewport(0,0,640,480);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,640,480,0,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

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
        image_draw(&gl, 0, 0, 640, 480);

        image_gl_destroy(&gl);

        SDL_GL_SwapBuffers();
    }

    return 0;
}

