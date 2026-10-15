#ifndef ECOS_HAL_INTERRUPT_H
#define ECOS_HAL_INTERRUPT_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*hal_interrupt_handler_t)(void *arg);

ecos_err_t hal_interrupt_init(void);
ecos_err_t hal_interrupt_alloc(uint32_t source,
                               uint32_t priority,
                               hal_interrupt_handler_t handler,
                               void *arg);
ecos_err_t hal_interrupt_free(uint32_t source);
ecos_err_t hal_interrupt_enable(uint32_t source);
ecos_err_t hal_interrupt_disable(uint32_t source);
void hal_interrupt_set_threshold(uint32_t threshold);
void hal_interrupt_global_enable(void);
void hal_interrupt_global_disable(void);

#ifdef __cplusplus
}
#endif

#endif
