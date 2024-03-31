#ifndef LOG_H
#define LOG_H

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO 3

void set_log_level(unsigned int level);

void log_with_level(unsigned int level, const char *str_template, ...);
void log_error(const char *str_template, ...);
void log_warning(const char *str_template, ...);
void log_info(const char *str_template, ...);

#endif // LOG_H