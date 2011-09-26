#ifndef __FILETYPE_H__
#define __FILETYPE_H__

#include <stdbool.h>
#include <inttypes.h>

#define FILETYPE_MAGIC_BYTES 8

// these functions assume there is at least FILETYPE_MAGIC_BYTES accessible
bool ft_is_rar(uint8_t *data);
bool ft_is_zip(uint8_t *data);
bool ft_is_jpg(uint8_t *data);
bool ft_is_png(uint8_t *data);
bool ft_is_gif(uint8_t *data);
bool ft_is_tif(uint8_t *data);
bool ft_is_pnm(uint8_t *data);
bool ft_is_bmp(uint8_t *data);

// meta-types
bool ft_is_archive(uint8_t *data);
bool ft_is_image(uint8_t *data);

// meta-helpers
bool ft_file_is_archive(char *path);
bool ft_file_is_image(char *path);

#endif