#include "filetype.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

static void ft_get8(char *path, uint8_t *data) {
    memset(data, 0, 8);
    FILE *fh = fopen(path, "rb");
    if ( fh == NULL ) return;
    fread(data, 1, 8, fh);
    fclose(fh);
}

bool ft_is_rar(uint8_t *data) {
    return data[0] == 'R' &&
           data[1] == 'a' &&
           data[2] == 'r' &&
           data[3] == '!';
}

bool ft_is_zip(uint8_t *data) {
    return data[0] == 'P' &&
           data[1] == 'K';
}

bool ft_is_jpg(uint8_t *data) {
    return data[0] == 0xff &&
           data[1] == 0xd8;
}

bool ft_is_png(uint8_t *data) {
    return data[0] == 0x89 &&
           data[1] == 'P' &&
           data[2] == 'N' &&
           data[3] == 'G' &&
           data[4] == 0x0d &&
           data[5] == 0x0a &&
           data[6] == 0x1a &&
           data[7] == 0x0a;
}

bool ft_is_gif(uint8_t *data) {
    return data[0] == 'G' &&
           data[1] == 'I' &&
           data[2] == 'F' &&
           data[3] == '8' &&
           (data[4] == '7' || data[4] == '9') &&
           data[5] == 'a';
}

bool ft_is_tif(uint8_t *data) {
    return (data[0] == 0x49 &&
            data[1] == 0x49 &&
            data[2] == 0x2a &&
            data[3] == 0x00)
        || 
           (data[0] == 0x4d &&
            data[1] == 0x4d &&
            data[2] == 0x00 &&
            data[3] == 0x2a);
}

bool ft_is_pnm(uint8_t *data) {
    return data[0] == 'P' &&
           data[1] >= '4' &&
           data[1] <= '6' &&
           isspace(data[2]);
}

bool ft_is_archive(uint8_t *data) {
    return ft_is_zip(data) ||
           ft_is_rar(data);
}

bool ft_is_image(uint8_t *data) {
    return ft_is_jpg(data) ||
           ft_is_png(data) ||
           ft_is_gif(data) ||
           ft_is_tif(data) ||
           ft_is_pnm(data);
}

bool ft_file_is_archive(char *path) {
    uint8_t data[8];
    ft_get8(path, data);
    return ft_is_archive(data);
}

bool ft_file_is_image(char *path) {
    uint8_t data[8];
    ft_get8(path, data);
    return ft_is_image(data);
}

