#include "filetype.h"
#include "inklog.h"
#define INKLOG_MODULE "filetype"

#include <errno.h>
#include <stdio.h>
#include <string.h>

bool ft_magic_matches_type(uint8_t *data, enum ft_type type) {
    switch ( type ) {
        case FT_RAR:
            return data[0] == 'R' &&
                   data[1] == 'a' &&
                   data[2] == 'r' &&
                   data[3] == '!';
        case FT_ZIP:
            return data[0] == 'P' &&
                   data[1] == 'K';
        case FT_JPEG:
            return data[0] == 0xff &&
                   data[1] == 0xd8;
        case FT_PNG:
            return data[0] == 0x89 &&
                   data[1] == 'P' &&
                   data[2] == 'N' &&
                   data[3] == 'G' &&
                   data[4] == 0x0d &&
                   data[5] == 0x0a &&
                   data[6] == 0x1a &&
                   data[7] == 0x0a;
        case FT_GIF:
            return data[0] == 'G' &&
                   data[1] == 'I' &&
                   data[2] == 'F' &&
                   data[3] == '8' &&
                   (data[4] == '7' || data[4] == '9') &&
                   data[5] == 'a';
        case FT_TIFF:
            return (data[0] == 0x49 &&
                    data[1] == 0x49 &&
                    data[2] == 0x2a &&
                    data[3] == 0x00)
                || 
                   (data[0] == 0x4d &&
                    data[1] == 0x4d &&
                    data[2] == 0x00 &&
                    data[3] == 0x2a);
        case FT_PNM:
            return data[0] == 'P' &&
                   data[1] >= '4' &&
                   data[1] <= '6' &&
                   (data[2] == ' ' || data[2] == '\t'
                    || data[2] == '\n' || data[2] == '\v'
                    || data[2] == '\f' || data[2] == '\r');
        case FT_BMP:
            return data[0] == 'B' &&
                   data[1] == 'M';
        case FT_ARCHIVE:
            return ft_magic_matches_type(data, FT_ZIP) ||
                   ft_magic_matches_type(data, FT_RAR);
        case FT_IMAGE:
            return ft_magic_matches_type(data, FT_JPEG) ||
                   ft_magic_matches_type(data, FT_PNG) ||
                   ft_magic_matches_type(data, FT_GIF) ||
                   ft_magic_matches_type(data, FT_TIFF) ||
                   ft_magic_matches_type(data, FT_PNM) ||
                   ft_magic_matches_type(data, FT_BMP);
    }
    return false;
}

bool ft_path_matches_type(char *path, enum ft_type type) {
    uint8_t data[FILETYPE_MAGIC_BYTES];
    memset(data, 0, FILETYPE_MAGIC_BYTES);

    FILE *fh = fopen(path, "rb");
    if ( fh == NULL ) {
        inklog(LOG_NOTICE, "Couldn't open %s for reading: %s", path, strerror(errno));
        return false;
    }

    // ignore errors
    fread(data, 1, FILETYPE_MAGIC_BYTES, fh);
    fclose(fh);

    return ft_magic_matches_type(data, type);
}
