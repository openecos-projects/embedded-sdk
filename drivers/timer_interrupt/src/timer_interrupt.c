#include "ecos/driver/timer_interrupt.h"

#include "ecos/hal/timer_interrupt.h"

#include <stddef.h>

static ecos_err_t timer_interrupt_map_result(int result)
{
    if (result >= 0)
        return (ecos_err_t)result;
    return ecos_err_is_known(result) ? (ecos_err_t)result : ECOS_ERR_IO;
}

ecos_err_t ecos_timer_register_callback(
    ecos_timer_id_t timer,
    ecos_timer_interrupt_handler_t callback,
    void *arg,
    uint32_t priority
)
{
    if (callback == NULL || priority == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    return timer_interrupt_map_result(
        hal_timer_register_callback(
            (hal_timer_id_t)timer,
            (hal_timer_interrupt_handler_t)callback,
            arg,
            priority
        )
    );
}

ecos_err_t ecos_timer_unregister_callback(ecos_timer_id_t timer)
{
    return timer_interrupt_map_result(
        hal_timer_unregister_callback((hal_timer_id_t)timer)
    );
}
