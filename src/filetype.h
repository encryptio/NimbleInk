#ifndef __FILETYPE_H__
#define __FILETYPE_H__

#include <stdbool.h>
#include <inttypes.h>

enum ft_type {
    FT_RAR,
    FT_ZIP,
    FT_JPEG,
    FT_PNG,
    FT_GIF,
    FT_TIFF,
    FT_PNM,
    FT_BMP,

    FT_ARCHIVE,
    FT_IMAGE
};

#define FILETYPE_MAGIC_BYTES 8

// data must have at least FILETYPE_MAGIC_BYTES accessible
bool ft_magic_matches_type(uint8_t *data, enum ft_type type);

bool ft_path_matches_type(char *path, enum ft_type type);

#endif
