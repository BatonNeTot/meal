#include "meal/log.h"

#include "meal/platform.h"
#include "meal/macros.h"

#include <string.h>
#include <sys/time.h>

int32_t logout_default(const char *buffer, uint32_t count, void *data) {
    if (fwrite(buffer, sizeof(char), count, stdout) < count)
        return -1;
    else
        return count;
}

int32_t logerr_default(const char *buffer, uint32_t count, void *data) {
    if (fwrite(buffer, sizeof(char), count, stderr) < count)
        return -1;
    else
        return count;
}

static writer_f logout = logout_default;
static writer_f logerr = logerr_default;

void logout_set(writer_f func) {
    if (func)
        logout = func;
}

inline writer_f logout_get() {
    return logout;
}

void logerr_set(writer_f func) {
    if (func)
        logerr = func;
}

inline writer_f logerr_get() {
    return logerr;
}

static FILE *log_file = NULL;
static bool log_file_trigger = true;

#define stdlog(buffer, count) \
do {\
    if (log_file_trigger) {\
        if (!log_file) {\
            log_file = fopen(LOG_FILENAME, "w");\
            if (!log_file) {\
                log_file_trigger = false;\
                log_fatal("Logging", "Can't open file '%s' for writing.", LOG_FILENAME);\
                break;\
            }\
        }\
        if (fwrite(buffer, sizeof(char), count, log_file) < count) {\
            log_file_trigger = false;\
            log_error("Logging", "Write error with file '%s'.", LOG_FILENAME);\
            log_file_trigger = true;\
        }\
    }\
} while(false)

static int32_t logout_f(const char *buffer, uint32_t count, void *data) {
    if (count == 0)
        return 0;
    stdlog(buffer, count);
    return logout(buffer, count, data);
}

static int32_t logerr_f(const char *buffer, uint32_t count, void *data) {
    if (count == 0)
        return 0;
    stdlog(buffer, count);
    return logerr(buffer, count, data);
}

int32_t print(const char *frmt, ...) {
    va_list args;
    va_start(args, frmt);
    int32_t count = wprintv(logout_f, NULL, frmt, args);
    if (log_file)
        fflush(log_file);
    va_end(args);
    if (count == -1)
        return -1;
    const int32_t result = wprint(logout_f, NULL, "\n");
    if (log_file)
        fflush(log_file);
    if (result == -1)
        return -1;
    return count + result;
}

static log_level _log_level = LOG_LEVEL_DEFAULT;

void log_level_set(log_level level) {
    if (level >= LOG_TRACE && level <= LOG_FATAL)
        _log_level = level;
}

inline log_level log_level_get() {
    return _log_level;
}

typedef struct {
    writer_f log;
    uint32_t spaceIndent;
    uint32_t horizontalCarret;
    bool wasEnter;
} log_info_t;

static int32_t customWriter(const char *buffer, uint32_t size, void *data) {
    log_info_t *info = data;
    uint32_t count = 0;
    while (size > 0) {
        if (info->wasEnter) {
            for (uint32_t i = 0; i < info->spaceIndent; i++) {
                const int32_t result = info->log(" ", 1, NULL);
                if (result == -1)
                    return -1;
                count += result;
            }
            info->wasEnter = 0;
        }
        switch (*buffer) {
            case '\n': {
                const int32_t result = info->log("\n", 1, NULL);
                if (result == -1)
                    return -1;
                count += result;

                info->wasEnter = 1;
                info->horizontalCarret = 0;
                break;
            }
            case '\t': {
                uint32_t tabCount = LOG_TAB_SIZE - (info->horizontalCarret % LOG_TAB_SIZE);
                info->horizontalCarret += tabCount;
                while (tabCount > 0) {
                    const int32_t result = info->log(" ", 1, NULL);
                    if (result == -1)
                        return -1;
                    count += result;
                    tabCount--;
                }
                break;
            }
            default: {
                const int32_t result = info->log(buffer, 1, NULL);
                if (result == -1)
                    return -1;
                count += result;
                info->horizontalCarret++;
            }
        }
        buffer++;
        size--;
    }
    return count;
}



static int32_t _log(log_level level, const char *tag, const char *frmt, va_list args) {
    if (level < _log_level)
        return 0;

    writer_f log = (writer_f) TERNP(level < LOG_WARNING, logout_f, logerr_f);

    struct timeval rawtime;
    struct tm *timeinfo;

    gettimeofday(&rawtime, NULL);
    timeinfo = localtime(&rawtime.tv_sec);

// ----TIME STAMP----

    int32_t spaceIndent = wprint(log, NULL, "[%02d:%02d:%02d:%03d]",
                              timeinfo->tm_hour,
                              timeinfo->tm_min,
                              timeinfo->tm_sec,
                              rawtime.tv_usec / 1000);

    if (spaceIndent == -1)
        return -1;

// ----TIME STAMP---- END

// ----LEVEL STAMP----

    char levelChar;
    switch (level) {
        case LOG_TRACE: {
            levelChar = 'T';
            break;
        }
        case LOG_DEBUG: {
            levelChar = 'D';
            break;
        }
        case LOG_INFO: {
            levelChar = 'I';
            break;
        }
        case LOG_WARNING: {
            levelChar = 'W';
            break;
        }
        case LOG_ERROR: {
            levelChar = 'E';
            break;
        }
        case LOG_FATAL: {
            levelChar = 'F';
            break;
        }
        default: {
            levelChar = 'U';
        }
    }

    {
        const int32_t result = wprint(log, NULL, "%c", levelChar);
        if (result == -1)
            return -1;
        spaceIndent += result;
    }

// ----LEVEL STAMP---- END

// ----TAG STAMP----

    const int32_t tagLength = (tag ? strlen(tag) : 0);
    if (tagLength > 0) {
        {
            const int32_t result = wprint(log, NULL, "\\");
            if (result == -1)
                return -1;
            spaceIndent += result;
        }

        {
            const int32_t result = wprint(log, NULL, tag);
            if (result == -1)
                return -1;
            spaceIndent += result;
        }
    }

// ----TAG STAMP---- END

    //Last character ":" if need to

    if (level != -1 || tagLength > 0) {
        const int32_t result = wprint(log, NULL, ": ");
        if (result == -1)
            return -1;
        spaceIndent += result;
    }

    //Ready to log message

    log_info_t info = {
            log: log,
            spaceIndent: (uint32_t) spaceIndent,
            horizontalCarret: 0,
            wasEnter: 0
    };

    int32_t count = spaceIndent;

    {
        const int32_t result = wprintv(customWriter, &info, frmt, args);
        if (result == -1)
            return -1;
        count += result;
    }

    if (!info.wasEnter) {
        const int32_t result = log("\n", 1, NULL);
        if (result == -1)
            return -1;
        count += result;
    }

    if (log_file)
        fflush(log_file);

    return count;
}

int32_t log_trace(const char *tag, const char *frmt, ...) {
    if (!frmt)
        return -1;

    va_list args;
    va_start(args, frmt);
    const int32_t result = _log(LOG_TRACE, tag, frmt, args);
    va_end(args);
    return result;
}

int32_t log_debug(const char *tag, const char *frmt, ...) {
    if (!frmt)
        return -1;

    va_list args;
    va_start(args, frmt);
    const int32_t result = _log(LOG_DEBUG, tag, frmt, args);
    va_end(args);
    return result;
}

int32_t log_info(const char *tag, const char *frmt, ...) {
    if (!frmt)
        return -1;

    va_list args;
    va_start(args, frmt);
    const int32_t result = _log(LOG_INFO, tag, frmt, args);
    va_end(args);
    return result;
}

int32_t log_warning(const char *tag, const char *frmt, ...) {
    if (!frmt)
        return -1;

    va_list args;
    va_start(args, frmt);
    const int32_t result = _log(LOG_WARNING, tag, frmt, args);
    va_end(args);
    return result;
}

int32_t log_error(const char *tag, const char *frmt, ...) {
    if (!frmt)
        return -1;

    va_list args;
    va_start(args, frmt);
    const int32_t result = _log(LOG_ERROR, tag, frmt, args);
    va_end(args);
    return result;
}

int32_t log_fatal(const char *tag, const char *frmt, ...) {
    if (!frmt)
        return -1;

    va_list args;
    va_start(args, frmt);
    const int32_t result = _log(LOG_FATAL, tag, frmt, args);
    va_end(args);
    return result;
}