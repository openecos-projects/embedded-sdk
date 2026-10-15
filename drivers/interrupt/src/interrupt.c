#include "ecos/driver/interrupt.h"

#include "ecos/hal/interrupt.h"

#include <stddef.h>

static ecos_err_t interrupt_map_result(int result)
{
    if (result >= 0)
        return (ecos_err_t)result;
    return ecos_err_is_known(result) ? (ecos_err_t)result : ECOS_ERR_IO;
}

ecos_err_t ecos_interrupt_init(void)
{
    return interrupt_map_result(hal_interrupt_init());
}

ecos_err_t ecos_interrupt_alloc(uint32_t source,
                                uint32_t priority,
                                ecos_interrupt_handler_t handler,
                                void *arg)
{
    if (handler == NULL || priority == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    return interrupt_map_result(
        hal_interrupt_alloc(source, priority,
                            (hal_interrupt_handler_t)handler, arg)
    );
}

ecos_err_t ecos_interrupt_free(uint32_t source)
{
    return interrupt_map_result(hal_interrupt_free(source));
}

ecos_err_t ecos_interrupt_enable(uint32_t source)
{
    return interrupt_map_result(hal_interrupt_enable(source));
}

ecos_err_t ecos_interrupt_disable(uint32_t source)
{
    return interrupt_map_result(hal_interrupt_disable(source));
}

void ecos_interrupt_set_threshold(uint32_t threshold)
{
    hal_interrupt_set_threshold(threshold);
}

void ecos_interrupt_global_enable(void)
{
    hal_interrupt_global_enable();
}

void ecos_interrupt_global_disable(void)
{
    hal_interrupt_global_disable();
}
