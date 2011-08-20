#ifndef __IMAGE_LIBJPEG_H__
#define __IMAGE_LIBJPEG_H__

#if ENABLE_LIBJPEG

#include "image.h"

#include <stdbool.h>

bool cpuimage_load_from_ram_libjpeg(void *ptr, int len, struct cpuimage *i);

#endif /* ENABLE_LIBJPEG */

#endif
