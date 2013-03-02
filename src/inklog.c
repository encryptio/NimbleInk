#include "inklog.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>

#define LOG_LINE_BUFFER_SIZE 1024

int inklog_level = LOG_INFO;

static const char * log_level_str(int priority) {
    switch ( priority ) {
        case LOG_EMERG:
            return "EMERG";
        case LOG_ALERT:
            return "ALERT";
        case LOG_CRIT:
            return "CRIT";
        case LOG_ERR:
            return "ERR";
        case LOG_WARNING:
            return "WARNING";
        case LOG_NOTICE:
            return "NOTICE";
        case LOG_INFO:
            return "INFO";
        case LOG_DEBUG:
            return "DEBUG";
        default:
            return "UNKNOWN";
    }
}

void inklog_real(int priority, const char *module, const char *msg, ...) {
    if ( priority < inklog_level ) return;

    va_list args;
    va_start(args, msg);

    char msgbuf[LOG_LINE_BUFFER_SIZE];
    char *next = msgbuf;
    int left = LOG_LINE_BUFFER_SIZE;

    int len;

#ifdef INKLOG_TIMESTAMPS
    // "[" timestamp "] "
    time_t clock = time(NULL);
    struct tm *tm = localtime(&clock);
    len = strftime(next, left-1, "[%Y-%m-%d %H:%M:%S] ", tm);
    assert(len != 0);
    left -= len;
    next += len;
    assert(left > 0);
#endif

    // "(" level ") " module ": "
    len = snprintf(next, left, "(%s) %s: ", log_level_str(priority), module);
    left -= len;
    next += len;
    assert(left > 0);

    // whatever message it was
    len = vsnprintf(next, left, msg, args);
    left -= len;
    next += len;
    assert(left > 0);

    // "\n"
    len = snprintf(next, left, "\n");
    left -= len;
    next += len;
    assert(left > 0);

    va_end(args);

    fprintf(stderr, "%s", msgbuf);

    // TODO: logfile
}
