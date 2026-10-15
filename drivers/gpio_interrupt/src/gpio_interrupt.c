#include "ecos/driver/gpio_interrupt.h"

#include "ecos/hal/gpio_interrupt.h"

#include <stddef.h>

static ecos_err_t gpio_interrupt_map_result(int result)
{
    if (result >= 0)
        return (ecos_err_t)result;
    return ecos_err_is_known(result) ? (ecos_err_t)result : ECOS_ERR_IO;
}

ecos_err_t ecos_gpio_interrupt_set_trigger(
    ecos_gpio_port_t port,
    uint8_t pin,
    ecos_gpio_interrupt_trigger_t trigger
)
{
    if (trigger < ECOS_GPIO_INTERRUPT_DISABLE ||
        trigger > ECOS_GPIO_INTERRUPT_HIGH_LEVEL)
        return ECOS_ERR_INVALID_ARGUMENT;
    return gpio_interrupt_map_result(
        hal_gpio_interrupt_set_trigger(
            (hal_gpio_port_t)port,
            pin,
            (hal_gpio_interrupt_trigger_t)trigger
        )
    );
}

ecos_err_t ecos_gpio_interrupt_handler_add(
    ecos_gpio_port_t port,
    uint8_t pin,
    ecos_gpio_interrupt_handler_t handler,
    void *arg,
    uint32_t priority
)
{
    if (handler == NULL || priority == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    return gpio_interrupt_map_result(
        hal_gpio_interrupt_handler_add(
            (hal_gpio_port_t)port,
            pin,
            (hal_gpio_interrupt_handler_t)handler,
            arg,
            priority
        )
    );
}

ecos_err_t ecos_gpio_interrupt_handler_remove(ecos_gpio_port_t port,
                                              uint8_t pin)
{
    return gpio_interrupt_map_result(
        hal_gpio_interrupt_handler_remove((hal_gpio_port_t)port, pin)
    );
}

ecos_err_t ecos_gpio_interrupt_enable(ecos_gpio_port_t port, uint8_t pin)
{
    return gpio_interrupt_map_result(
        hal_gpio_interrupt_enable((hal_gpio_port_t)port, pin)
    );
}

ecos_err_t ecos_gpio_interrupt_disable(ecos_gpio_port_t port, uint8_t pin)
{
    return gpio_interrupt_map_result(
        hal_gpio_interrupt_disable((hal_gpio_port_t)port, pin)
    );
}
