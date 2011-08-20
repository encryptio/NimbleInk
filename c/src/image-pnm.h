#ifndef __IMAGE_PNM_H__
#define __IMAGE_PNM_H__

#if ENABLE_PNM

#include "image.h"

#include <stdbool.h>

bool cpuimage_load_from_ram_pnm(void *orig, int len, struct cpuimage *i);

#endif /* ENABLE_PNM */

#endif
