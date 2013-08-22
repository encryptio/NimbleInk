#ifndef __INKLOG_H__
#define __INKLOG_H__

#include <syslog.h>
#include <stdarg.h>

extern int inklog_level;

#define inklog(prio, args...) \
    inklog_real(prio, INKLOG_MODULE, args)

void inklog_real(int priority, const char *module, const char *msg, ...)
    __attribute__ ((format (printf, 3, 4)));

#endif
