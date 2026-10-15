#ifndef ECOS_HAL_CLINT_H
#define ECOS_HAL_CLINT_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t low;
    uint32_t high;
} hal_clint_value_t;

/* Return the target's mtime clock frequency in Hz. */
uint32_t hal_clint_get_frequency_hz(void);

ecos_err_t hal_clint_get_mtime(hal_clint_value_t *value);
ecos_err_t hal_clint_set_mtime(const hal_clint_value_t *value);
ecos_err_t hal_clint_get_mtimecmp(hal_clint_value_t *value);
ecos_err_t hal_clint_set_mtimecmp(const hal_clint_value_t *value);

#ifdef __cplusplus
}
#endif

#endif
