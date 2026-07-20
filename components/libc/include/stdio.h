#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdarg.h>
#include <stddef.h>
#include "hal_sys_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Formatted output to stdout
 * @param fmt Format string
 * @param ... Variable arguments
 * @return Number of characters written
 */
int printf(const char *fmt, ...);

/**
 * @brief Write a string followed by a newline to stdout
 * @param str Null-terminated string to write
 * @return Number of characters written
 */
int puts(const char *str);

/**
 * @brief Formatted output with va_list
 * @param fmt Format string
 * @param args Variable argument list
 * @return Number of characters written
 */
int vprintf(const char *fmt, va_list args);

int vsnprintf(char *str, size_t size, const char *fmt, va_list args);

int snprintf(char *buffer, size_t size, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
