#ifndef __IMAGE_GIFLIB_H__
#define __IMAGE_GIFLIB_H__

#include "image.h"

#include <stdbool.h>

bool image_load_from_ram_giflib(void *ptr, int len, struct cpuimage *i);

#endif
