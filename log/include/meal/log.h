#ifndef MEAL_LOG_H
#define MEAL_LOG_H

#include "meal/print.h"

enum log_level {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL,
};

typedef enum log_level log_level;

#ifndef LOG_LEVEL_DEFAULT
#define LOG_LEVEL_DEFAULT LOG_TRACE
#endif // LOG_LEVEL_DEFAULT

#ifndef LOG_FILENAME
#define LOG_FILENAME "log.txt"
#endif // LOG_FILENAME

#ifndef LOG_TAB_SIZE
#define LOG_TAB_SIZE 8
#endif // LOG_TAB_SIZE

int32_t logout_default(const char *buffer, uint32_t count, void *data);
int32_t logerr_default(const char *buffer, uint32_t count, void *data);

void logout_set(writer_f func);
writer_f logout_get();

void logerr_set(writer_f func);
writer_f logerr_get();

int32_t print(const char *frmt, ...);

void log_level_set(log_level level);
log_level log_level_get();

int32_t log_trace(const char *tag, const char *frmt, ...);
int32_t log_debug(const char *tag, const char *frmt, ...);
int32_t log_info(const char *tag, const char *frmt, ...);
int32_t log_warning(const char *tag, const char *frmt, ...);
int32_t log_error(const char *tag, const char *frmt, ...);
int32_t log_fatal(const char *tag, const char *frmt, ...);

#endif // MEAL_LOG_H