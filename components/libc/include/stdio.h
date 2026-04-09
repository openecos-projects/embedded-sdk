#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdarg.h>
#include <stddef.h>
#include "sys_uart.h"

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
 * @brief Formatted output with va_list
 * @param fmt Format string
 * @param args Variable argument list
 * @return Number of characters written
 */
int vprintf(const char *fmt, va_list args);

#ifdef __cplusplus
}
#endif

#endif