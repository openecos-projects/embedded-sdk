#ifndef ECOS_DRIVER_CLINT_H
#define ECOS_DRIVER_CLINT_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t low;
    uint32_t high;
} ecos_clint_value_t;

uint32_t ecos_clint_get_frequency_hz(void);
ecos_err_t ecos_clint_get_mtime(ecos_clint_value_t *value);
ecos_err_t ecos_clint_set_mtime(const ecos_clint_value_t *value);
ecos_err_t ecos_clint_get_mtimecmp(ecos_clint_value_t *value);
ecos_err_t ecos_clint_set_mtimecmp(const ecos_clint_value_t *value);

#ifdef __cplusplus
}
#endif

#endif
