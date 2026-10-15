#include "ecos/hal/gpio_interrupt.h"
#include "ecos/hal/interrupt.h"
#include "cl1_2512_soc.h"

#include <stddef.h>
#include <stdint.h>

static hal_gpio_interrupt_handler_t gpio_handlers[CL1_2512_GPIO_PINS_PER_PORT];
static void *gpio_handler_args[CL1_2512_GPIO_PINS_PER_PORT];

static int gpio_interrupt_pin_is_valid(hal_gpio_port_t port, uint8_t pin)
{
    return port == HAL_GPIO_PORT_0 && pin < CL1_2512_GPIO_PINS_PER_PORT;
}

static uint32_t gpio_interrupt_source(uint8_t pin)
{
    return 8u + pin;
}

static void gpio_interrupt_dispatch(void *arg)
{
    uint8_t pin = (uint8_t)(uintptr_t)arg;
    uint32_t mask = 1u << pin;
    hal_gpio_interrupt_handler_t handler;

    REG_GPIO_0_PORTA_EOI = mask;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    handler = gpio_handlers[pin];
    if (handler != NULL)
        handler(gpio_handler_args[pin]);
}

ecos_err_t hal_gpio_interrupt_set_trigger(
    hal_gpio_port_t port,
    uint8_t pin,
    hal_gpio_interrupt_trigger_t trigger
)
{
    uint32_t mask;

    if (!gpio_interrupt_pin_is_valid(port, pin) ||
        trigger > HAL_GPIO_INTERRUPT_HIGH_LEVEL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (trigger == HAL_GPIO_INTERRUPT_DISABLE)
        return hal_gpio_interrupt_disable(port, pin);

    mask = 1u << pin;
    REG_GPIO_0_SWPORTA_CTL &= ~mask;
    REG_GPIO_0_SWPORTA_DDR &= ~mask;
    REG_GPIO_0_DEBOUNCE &= ~mask;
    REG_GPIO_0_INT_BOTHEDGE &= ~mask;
    if (trigger == HAL_GPIO_INTERRUPT_POSEDGE ||
        trigger == HAL_GPIO_INTERRUPT_NEGEDGE ||
        trigger == HAL_GPIO_INTERRUPT_ANYEDGE)
        REG_GPIO_0_INTTYPE_LEVEL |= mask;
    else
        REG_GPIO_0_INTTYPE_LEVEL &= ~mask;
    if (trigger == HAL_GPIO_INTERRUPT_POSEDGE ||
        trigger == HAL_GPIO_INTERRUPT_HIGH_LEVEL)
        REG_GPIO_0_INT_POLARITY |= mask;
    else
        REG_GPIO_0_INT_POLARITY &= ~mask;
    if (trigger == HAL_GPIO_INTERRUPT_ANYEDGE)
        REG_GPIO_0_INT_BOTHEDGE |= mask;
    REG_GPIO_0_PORTA_EOI = mask;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return ECOS_OK;
}

ecos_err_t hal_gpio_interrupt_handler_add(
    hal_gpio_port_t port,
    uint8_t pin,
    hal_gpio_interrupt_handler_t handler,
    void *arg,
    uint32_t priority
)
{
    uint32_t source;

    if (!gpio_interrupt_pin_is_valid(port, pin) || handler == NULL ||
        priority == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (gpio_handlers[pin] != NULL)
        return ECOS_ERR_BUSY;
    gpio_handler_args[pin] = arg;
    gpio_handlers[pin] = handler;
    source = gpio_interrupt_source(pin);
    if (hal_interrupt_alloc(source, priority, gpio_interrupt_dispatch,
                             (void *)(uintptr_t)pin) != ECOS_OK) {
        gpio_handlers[pin] = NULL;
        gpio_handler_args[pin] = NULL;
        return ECOS_ERR_IO;
    }
    return ECOS_OK;
}

ecos_err_t hal_gpio_interrupt_handler_remove(hal_gpio_port_t port,
                                             uint8_t pin)
{
    uint32_t source;

    if (!gpio_interrupt_pin_is_valid(port, pin) || gpio_handlers[pin] == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    (void)hal_gpio_interrupt_disable(port, pin);
    source = gpio_interrupt_source(pin);
    if (hal_interrupt_free(source) != ECOS_OK)
        return ECOS_ERR_IO;
    gpio_handlers[pin] = NULL;
    gpio_handler_args[pin] = NULL;
    return ECOS_OK;
}

ecos_err_t hal_gpio_interrupt_enable(hal_gpio_port_t port, uint8_t pin)
{
    uint32_t mask;
    uint32_t source;

    if (!gpio_interrupt_pin_is_valid(port, pin) || gpio_handlers[pin] == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    mask = 1u << pin;
    REG_GPIO_0_PORTA_EOI = mask;
    REG_GPIO_0_INTEN |= mask;
    REG_GPIO_0_INTMASK &= ~mask;
    source = gpio_interrupt_source(pin);
    if (hal_interrupt_enable(source) != ECOS_OK) {
        REG_GPIO_0_INTMASK |= mask;
        REG_GPIO_0_INTEN &= ~mask;
        return ECOS_ERR_IO;
    }
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return ECOS_OK;
}

ecos_err_t hal_gpio_interrupt_disable(hal_gpio_port_t port, uint8_t pin)
{
    uint32_t mask;
    uint32_t source;

    if (!gpio_interrupt_pin_is_valid(port, pin))
        return ECOS_ERR_INVALID_ARGUMENT;
    mask = 1u << pin;
    REG_GPIO_0_INTMASK |= mask;
    REG_GPIO_0_INTEN &= ~mask;
    REG_GPIO_0_PORTA_EOI = mask;
    source = gpio_interrupt_source(pin);
    (void)hal_interrupt_disable(source);
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return ECOS_OK;
}
