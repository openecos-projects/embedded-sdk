#ifndef ECOS_LEGACY_LOG_H
#define ECOS_LEGACY_LOG_H

#include "ecos/log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef ecos_log_level_t LogLevel;

#define LOG_DEBUG ECOS_LOG_DEBUG
#define LOG_INFO  ECOS_LOG_INFO
#define LOG_WARN  ECOS_LOG_WARN
#define LOG_ERROR ECOS_LOG_ERROR
#define LOG_FATAL ECOS_LOG_FATAL

#if defined(__GNUC__) || defined(__clang__)
#define ECOS_LEGACY_LOG_DEPRECATED(message) \
    __attribute__((deprecated(message)))
#else
#define ECOS_LEGACY_LOG_DEPRECATED(message)
#endif

ECOS_LEGACY_LOG_DEPRECATED("use ecos_log_set_level")
void log_init(LogLevel level, const char *filename);
ECOS_LEGACY_LOG_DEPRECATED("use the ECOS_LOG* macros")
void log_print(LogLevel level,
               const char *file,
               int line,
               const char *format,
               ...);
ECOS_LEGACY_LOG_DEPRECATED("the SDK logger does not require closing")
void log_close(void);

#define log_debug(format, ...) \
    log_print(LOG_DEBUG, __FILE__, __LINE__, (format), ##__VA_ARGS__)
#define log_info(format, ...) \
    log_print(LOG_INFO, __FILE__, __LINE__, (format), ##__VA_ARGS__)
#define log_warn(format, ...) \
    log_print(LOG_WARN, __FILE__, __LINE__, (format), ##__VA_ARGS__)
#define log_error(format, ...) \
    log_print(LOG_ERROR, __FILE__, __LINE__, (format), ##__VA_ARGS__)
#define log_fatal(format, ...) \
    log_print(LOG_FATAL, __FILE__, __LINE__, (format), ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
