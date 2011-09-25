#ifndef __ARCHIVE_UNZIP_H__
#define __ARCHIVE_UNZIP_H__

#include "archive.h"

bool archive_load_toc_zip(struct archive *ar);
bool archive_load_all_zip(struct archive *ar);
bool archive_load_single_zip(struct archive *ar, int which);

#endif

