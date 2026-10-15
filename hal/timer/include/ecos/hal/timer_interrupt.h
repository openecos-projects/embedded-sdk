#ifndef ECOS_HAL_TIMER_INTERRUPT_H
#define ECOS_HAL_TIMER_INTERRUPT_H

#include "ecos/error.h"
#include "ecos/hal/timer.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*hal_timer_interrupt_handler_t)(void *arg);

ecos_err_t hal_timer_clear_interrupt(hal_timer_id_t timer);
ecos_err_t hal_timer_register_callback(
    hal_timer_id_t timer,
    hal_timer_interrupt_handler_t callback,
    void *arg,
    uint32_t priority
);
ecos_err_t hal_timer_unregister_callback(hal_timer_id_t timer);

#ifdef __cplusplus
}
#endif

#endif
