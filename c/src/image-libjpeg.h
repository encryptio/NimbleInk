#ifndef __IMAGE_LIBJPEG_H__
#define __IMAGE_LIBJPEG_H__

#include "image.h"

#include <stdbool.h>

bool image_load_from_ram_libjpeg(void *ptr, int len, struct cpuimage *i);

#endif
