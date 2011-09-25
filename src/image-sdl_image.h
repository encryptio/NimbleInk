#ifndef __IMAGE_SDL_IMAGE_H__
#define __IMAGE_SDL_IMAGE_H__

#if ENABLE_SDL_IMAGE

#include "image.h"

#include <stdbool.h>

bool cpuimage_load_from_ram_sdl_image(void *ptr, int len, struct cpuimage *i);

#endif /* ENABLE_SDL_IMAGE */

#endif
