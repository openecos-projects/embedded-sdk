#ifndef ECOS_DRIVER_INTERRUPT_H
#define ECOS_DRIVER_INTERRUPT_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ecos_interrupt_handler_t)(void *arg);

ecos_err_t ecos_interrupt_init(void);
ecos_err_t ecos_interrupt_alloc(uint32_t source,
                                uint32_t priority,
                                ecos_interrupt_handler_t handler,
                                void *arg);
ecos_err_t ecos_interrupt_free(uint32_t source);
ecos_err_t ecos_interrupt_enable(uint32_t source);
ecos_err_t ecos_interrupt_disable(uint32_t source);
void ecos_interrupt_set_threshold(uint32_t threshold);
void ecos_interrupt_global_enable(void);
void ecos_interrupt_global_disable(void);

#ifdef __cplusplus
}
#endif

#endif
