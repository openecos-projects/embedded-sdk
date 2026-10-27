#ifndef ECOS_HAL_SYSTICK_H
#define ECOS_HAL_SYSTICK_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*hal_systick_callback_t)(void *arg);

typedef struct {
    uint64_t compare_ticks;
} hal_systick_config_t;

ecos_err_t hal_systick_init(const hal_systick_config_t *config,
                            hal_systick_callback_t callback, void *arg,
                            uint32_t priority);
ecos_err_t hal_systick_start(void);
ecos_err_t hal_systick_stop(void);
ecos_err_t hal_systick_get_counter(uint64_t *counter);

#ifdef __cplusplus
}
#endif

#endif
