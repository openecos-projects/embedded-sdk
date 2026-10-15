#ifndef ECOS_DRIVER_GPIO_INTERRUPT_H
#define ECOS_DRIVER_GPIO_INTERRUPT_H

#include "ecos/error.h"
#include "ecos/driver/gpio.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECOS_GPIO_INTERRUPT_DISABLE = 0,
    ECOS_GPIO_INTERRUPT_POSEDGE,
    ECOS_GPIO_INTERRUPT_NEGEDGE,
    ECOS_GPIO_INTERRUPT_ANYEDGE,
    ECOS_GPIO_INTERRUPT_LOW_LEVEL,
    ECOS_GPIO_INTERRUPT_HIGH_LEVEL
} ecos_gpio_interrupt_trigger_t;

typedef void (*ecos_gpio_interrupt_handler_t)(void *arg);

ecos_err_t ecos_gpio_interrupt_set_trigger(
    ecos_gpio_port_t port,
    uint8_t pin,
    ecos_gpio_interrupt_trigger_t trigger
);
ecos_err_t ecos_gpio_interrupt_handler_add(
    ecos_gpio_port_t port,
    uint8_t pin,
    ecos_gpio_interrupt_handler_t handler,
    void *arg,
    uint32_t priority
);
ecos_err_t ecos_gpio_interrupt_handler_remove(ecos_gpio_port_t port,
                                              uint8_t pin);
ecos_err_t ecos_gpio_interrupt_enable(ecos_gpio_port_t port, uint8_t pin);
ecos_err_t ecos_gpio_interrupt_disable(ecos_gpio_port_t port, uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif
