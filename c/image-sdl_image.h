#ifndef __IMAGE_SDL_IMAGE_H__
#define __IMAGE_SDL_IMAGE_H__

#include "image.h"

#include <stdbool.h>

bool image_load_from_ram_sdl_image(void *ptr, int len, struct cpuimage *i);

#endif
