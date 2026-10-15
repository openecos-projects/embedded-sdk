#ifndef ECOS_BSP_CONSOLE_H
#define ECOS_BSP_CONSOLE_H

#include "ecos/error.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the board-selected console. Returns 0 or a negative error code. */
ecos_err_t bsp_console_init(void);

/* Text output uses CRLF and returns input bytes consumed or a negative error. */
int bsp_console_write(const char *text, size_t size);

/* Blocking text input. Carriage returns are normalized to newlines. */
int bsp_console_read(void *data, size_t size);

/* Returns 1, 0, or a negative error; carriage return becomes newline. */
int bsp_console_try_read(uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif
