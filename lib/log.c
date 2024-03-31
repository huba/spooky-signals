#include "log.h"

#include <stdarg.h>
#include <stdio.h>

static unsigned int log_level = LOG_LEVEL_NONE;

void set_log_level(unsigned int level) {
    log_level = level;
}

void log_with_level(unsigned int level, const char *str_template, ...) {
    if (level > log_level) return;

    va_list p;
    va_start(p, str_template);
    vfprintf(stderr, str_template, p);
    va_end(p);
}

void log_error(const char *str_template, ...) {
    if (LOG_LEVEL_ERROR > log_level) return;

    va_list p;
    va_start(p, str_template);
    vfprintf(stderr, str_template, p);
    va_end(p);
}

void log_warning(const char *str_template, ...) {
    if (LOG_LEVEL_WARNING > log_level) return;

    va_list p;
    va_start(p, str_template);
    vfprintf(stderr, str_template, p);
    va_end(p);
}

void log_info(const char *str_template, ...) {
    if (LOG_LEVEL_INFO > log_level) return;

    va_list p;
    va_start(p, str_template);
    vfprintf(stderr, str_template, p);
    va_end(p);
}