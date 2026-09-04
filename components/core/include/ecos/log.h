#ifndef ECOS_LOG_H
#define ECOS_LOG_H

#include "ecos/error.h"

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECOS_LOG_DEBUG = 0,
    ECOS_LOG_INFO,
    ECOS_LOG_WARN,
    ECOS_LOG_ERROR,
    ECOS_LOG_FATAL,
    ECOS_LOG_OFF
} ecos_log_level_t;

/* Writers return bytes accepted, or a negative ECOS error code. */
typedef int (*ecos_log_writer_t)(void *context,
                                 const char *data,
                                 size_t size);
typedef void (*ecos_panic_handler_t)(void *context);

/* The first release targets a single-threaded bare-metal runtime. Logging from
 * interrupt handlers is not supported unless the installed writer supports it. */
ecos_err_t ecos_log_set_level(ecos_log_level_t level);
ecos_log_level_t ecos_log_get_level(void);
ecos_err_t ecos_log_set_writer(ecos_log_writer_t writer, void *context);

int ecos_log_vwrite(ecos_log_level_t level,
                    const char *tag,
                    const char *file,
                    int line,
                    const char *format,
                    va_list arguments);
int ecos_log_write(ecos_log_level_t level,
                   const char *tag,
                   const char *file,
                   int line,
                   const char *format,
                   ...);
int ecos_log_error(const char *tag,
                   ecos_err_t error,
                   const char *operation,
                   const char *file,
                   int line);

void ecos_panic_set_handler(ecos_panic_handler_t handler, void *context);
#if defined(__GNUC__) || defined(__clang__)
__attribute__((noreturn))
#endif
void ecos_panic(void);

#ifndef CONFIG_ECOS_LOG_LEVEL
#define CONFIG_ECOS_LOG_LEVEL 1
#endif

#if CONFIG_ECOS_LOG_LEVEL <= 0
#define ECOS_LOGD(tag, ...) \
    ecos_log_write(ECOS_LOG_DEBUG, (tag), __FILE__, __LINE__, __VA_ARGS__)
#else
#define ECOS_LOGD(tag, ...) (ECOS_OK)
#endif

#if CONFIG_ECOS_LOG_LEVEL <= 1
#define ECOS_LOGI(tag, ...) \
    ecos_log_write(ECOS_LOG_INFO, (tag), __FILE__, __LINE__, __VA_ARGS__)
#else
#define ECOS_LOGI(tag, ...) (ECOS_OK)
#endif

#if CONFIG_ECOS_LOG_LEVEL <= 2
#define ECOS_LOGW(tag, ...) \
    ecos_log_write(ECOS_LOG_WARN, (tag), __FILE__, __LINE__, __VA_ARGS__)
#else
#define ECOS_LOGW(tag, ...) (ECOS_OK)
#endif

#if CONFIG_ECOS_LOG_LEVEL <= 3
#define ECOS_LOGE(tag, ...) \
    ecos_log_write(ECOS_LOG_ERROR, (tag), __FILE__, __LINE__, __VA_ARGS__)
#define ECOS_LOG_ERR(tag, error, operation) \
    ecos_log_error((tag), (error), (operation), __FILE__, __LINE__)
#else
#define ECOS_LOGE(tag, ...) (ECOS_OK)
#define ECOS_LOG_ERR(tag, error, operation) (ECOS_OK)
#endif

#if CONFIG_ECOS_LOG_LEVEL <= 4
#define ECOS_LOGF(tag, ...) \
    ecos_log_write(ECOS_LOG_FATAL, (tag), __FILE__, __LINE__, __VA_ARGS__)
#else
#define ECOS_LOGF(tag, ...) (ECOS_OK)
#endif

#define ECOS_PANIC(tag, ...) \
    do { \
        (void)ECOS_LOGF((tag), __VA_ARGS__); \
        ecos_panic(); \
    } while (0)

/* Evaluate result_expression once, then report and halt on failure. */
#define ECOS_PANIC_ON_ERROR(tag, result_expression, operation) \
    do { \
        const int _ecos_cf_result = (int)(result_expression); \
        if (ecos_result_failed(_ecos_cf_result)) { \
            (void)ECOS_LOG_ERR((tag), \
                               (ecos_err_t)_ecos_cf_result, \
                               (operation)); \
            ecos_panic(); \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif
