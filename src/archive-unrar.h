#ifndef __ARCHIVE_UNRAR_H__
#define __ARCHIVE_UNRAR_H__

#include "archive.h"

bool archive_load_toc_rar(struct archive *ar);
bool archive_load_all_rar(struct archive *ar);
bool archive_load_single_rar(struct archive *ar, int which);

#endif
