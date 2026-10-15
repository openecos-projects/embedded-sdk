#ifndef ECOS_DRIVER_TIMER_INTERRUPT_H
#define ECOS_DRIVER_TIMER_INTERRUPT_H

#include "ecos/error.h"
#include "ecos/driver/timer.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ecos_timer_interrupt_handler_t)(void *arg);

ecos_err_t ecos_timer_register_callback(
    ecos_timer_id_t timer,
    ecos_timer_interrupt_handler_t callback,
    void *arg,
    uint32_t priority
);
ecos_err_t ecos_timer_unregister_callback(ecos_timer_id_t timer);

#ifdef __cplusplus
}
#endif

#endif
