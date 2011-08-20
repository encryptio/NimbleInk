#ifndef __STRINGUTILS_H__
#define __STRINGUTILS_H__

// for size_t
#include <stdio.h>

void str_append_quoted(char *dst, const char *src, size_t n);
void str_append(char *dst, const char *src, size_t n);

#endif
