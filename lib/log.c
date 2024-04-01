#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int log_level = LOG_LEVEL_NONE;

void set_log_level(unsigned int level) {
    log_level = level;
}

void get_log_env() {
    const char *env = getenv("LOG_LEVEL");

    if (!env) {
        log_level = LOG_LEVEL_ERROR;
        return;
    }

    if (strncmp(env, "none", 4) == 0) {
        log_level = LOG_LEVEL_NONE;
    } else if(strncmp(env, "error", 5) == 0) {
        log_level = LOG_LEVEL_ERROR;
    } else if (strncmp(env, "warning", 7) == 0) {
        log_level = LOG_LEVEL_WARNING;
    } else if (strncmp(env, "info", 4) == 0) {
        log_level = LOG_LEVEL_INFO;
    }
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