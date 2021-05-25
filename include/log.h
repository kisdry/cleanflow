#ifndef LOG_H
#define LOG_H

#include <libgen.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

enum log_level {
  LOG_LEVEL_NONE = 5,
  LOG_LEVEL_ERR = 4,
  LOG_LEVEL_WARN = 3,
  LOG_LEVEL_INFO = 2,
  LOG_LEVEL_DEBUG = 1
};

#define COLOR_NONE "\e[0m"
#define COLOR_RED "\e[1;31m"
#define COLOR_GREEN "\e[1;32m"
#define COLOR_YELLOW "\e[1;33m"
#define COLOR_BLUE "\e[1;38m"

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
#endif

#define log_debug(...) vlog(LOG_LEVEL_DEBUG, "D", COLOR_BLUE, __VA_ARGS__)
#define log_info(...) vlog(LOG_LEVEL_INFO, "I", COLOR_GREEN, __VA_ARGS__)
#define log_warn(...) vlog(LOG_LEVEL_WARN, "W", COLOR_YELLOW, __VA_ARGS__)
#define log_error(...) vlog(LOG_LEVEL_ERR, "E", COLOR_RED, __VA_ARGS__)

#define vlog(level, levelname, color, ...)                                     \
  if (level >= LOG_LEVEL) {                                                    \
    char strtime[20] = {0};                                                    \
    time_t timep;                                                              \
    struct tm p_tm;                                                            \
    char file[] = __FILE__;                                                    \
                                                                               \
    timep = time(NULL);                                                        \
    localtime_r(&timep, &p_tm);                                                \
    strftime(strtime, sizeof(strtime), "%Y-%m-%d %H:%M:%S", &p_tm);            \
    fprintf(stderr, "%s%s [%s] %s:%d <%s> ", strtime, color, levelname,       \
            basename(file), __LINE__, __FUNCTION__);                           \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "%s\n", COLOR_NONE);                                       \
  }

#endif
