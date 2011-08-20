#ifndef __IMAGE_GIFLIB_H__
#define __IMAGE_GIFLIB_H__

#if ENABLE_GIFLIB

#include "image.h"

#include <stdbool.h>

bool cpuimage_load_from_ram_giflib(void *ptr, int len, struct cpuimage *i);

#endif /* ENABLE_GIFLIB */

#endif
