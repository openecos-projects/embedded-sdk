#ifndef ECOS_HAL_GPIO_INTERRUPT_H
#define ECOS_HAL_GPIO_INTERRUPT_H

#include "ecos/error.h"
#include "ecos/hal/gpio.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_GPIO_INTERRUPT_DISABLE = 0,
    HAL_GPIO_INTERRUPT_POSEDGE,
    HAL_GPIO_INTERRUPT_NEGEDGE,
    HAL_GPIO_INTERRUPT_ANYEDGE,
    HAL_GPIO_INTERRUPT_LOW_LEVEL,
    HAL_GPIO_INTERRUPT_HIGH_LEVEL
} hal_gpio_interrupt_trigger_t;

typedef void (*hal_gpio_interrupt_handler_t)(void *arg);

ecos_err_t hal_gpio_interrupt_set_trigger(
    hal_gpio_port_t port,
    uint8_t pin,
    hal_gpio_interrupt_trigger_t trigger
);
ecos_err_t hal_gpio_interrupt_handler_add(
    hal_gpio_port_t port,
    uint8_t pin,
    hal_gpio_interrupt_handler_t handler,
    void *arg,
    uint32_t priority
);
ecos_err_t hal_gpio_interrupt_handler_remove(hal_gpio_port_t port,
                                             uint8_t pin);
ecos_err_t hal_gpio_interrupt_enable(hal_gpio_port_t port, uint8_t pin);
ecos_err_t hal_gpio_interrupt_disable(hal_gpio_port_t port, uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif
