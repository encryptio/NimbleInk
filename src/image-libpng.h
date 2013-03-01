#ifndef __IMAGE_LIBPNG_H__
#define __IMAGE_LIBPNG_H__

#if ENABLE_LIBPNG

#include "image.h"

#include <stdbool.h>

bool cpuimage_load_from_ram_libpng(void *ptr, int len, struct cpuimage *i);

#endif /* ENABLE_LIBPNG */

#endif
