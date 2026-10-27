#ifndef ECOS_DRIVER_SYSTICK_H
#define ECOS_DRIVER_SYSTICK_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ecos_systick_callback_t)(void *arg);
typedef struct { uint64_t compare_ticks; } ecos_systick_config_t;

ecos_err_t ecos_systick_init(const ecos_systick_config_t *config,
                             ecos_systick_callback_t callback, void *arg,
                             uint32_t priority);
ecos_err_t ecos_systick_start(void);
ecos_err_t ecos_systick_stop(void);
ecos_err_t ecos_systick_get_counter(uint64_t *counter);

#ifdef __cplusplus
}
#endif

#endif
