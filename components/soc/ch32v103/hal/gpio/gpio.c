/* Minimal CH32V103 GPIO backend for GPIOA..GPIOD pins 0..15.
 * GPIOA0/A1 are used by the CH32V103C8T6 gpio-demo Board resource.
 * Register layout follows the WCH CH32V103 EVT ch32v10x.h definitions.
 */
#include "ecos/hal/gpio.h"

#include <stddef.h>
#include <stdint.h>

#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))

#define RCC_APB2PCENR REG32(0x40021018u)
#define GPIOA_BASE    0x40010800u
#define GPIO_PORT_STRIDE 0x400u
#define GPIO_CFGLR_OFFSET 0x00u
#define GPIO_CFGHR_OFFSET 0x04u
#define GPIO_INDR_OFFSET  0x08u
#define GPIO_OUTDR_OFFSET 0x0Cu
#define GPIO_BSHR_OFFSET  0x10u
#define GPIO_BCR_OFFSET   0x14u

#define GPIO_INPUT_MODE  0u
#define GPIO_OUTPUT_MODE 3u /* 50 MHz output mode */
#define GPIO_CNF_INPUT_FLOATING 1u
#define GPIO_CNF_OUTPUT_PUSHPULL 0u
#define GPIO_CNF_OUTPUT_ALTERNATE_PUSHPULL 2u
#define GPIO_CNF_OUTPUT_ALTERNATE_OPENDRAIN 3u

static int valid_pin(hal_gpio_port_t port, uint8_t pin)
{
    return port >= HAL_GPIO_PORT_0 && port < HAL_GPIO_PORT_COUNT && pin < 16u;
}

static uintptr_t port_base(hal_gpio_port_t port)
{
    return GPIOA_BASE + (uintptr_t)port * GPIO_PORT_STRIDE;
}

static volatile uint32_t *port_register(hal_gpio_port_t port,
                                         uint32_t offset)
{
    return (volatile uint32_t *)(port_base(port) + offset);
}

static volatile uint32_t *config_register(hal_gpio_port_t port, uint8_t pin)
{
    return port_register(
        port, pin < 8u ? GPIO_CFGLR_OFFSET : GPIO_CFGHR_OFFSET
    );
}

static uint32_t config_shift(uint8_t pin)
{
    return (uint32_t)(pin & 7u) * 4u;
}

static void enable_port_clock(hal_gpio_port_t port)
{
    /* RCC_APB2PCENR: IOPAEN starts at bit 2 and is contiguous for A..G. */
    RCC_APB2PCENR |= 1u << (2u + (uint32_t)port);
}

static uint32_t read_config(hal_gpio_port_t port, uint8_t pin)
{
    return (*config_register(port, pin) >> config_shift(pin)) & 0xFu;
}

static void write_config(hal_gpio_port_t port, uint8_t pin, uint32_t value)
{
    volatile uint32_t *reg = config_register(port, pin);
    uint32_t shift = config_shift(pin);
    uint32_t current = *reg;

    *reg = (current & ~(0xFu << shift)) | ((value & 0xFu) << shift);
}

static uint32_t output_config(hal_gpio_direction_t direction,
                              hal_gpio_function_t function)
{
    if (direction == HAL_GPIO_DIRECTION_INPUT) {
        if (function != HAL_GPIO_FUNCTION_GPIO)
            return 0xFFFFFFFFu;
        return (GPIO_INPUT_MODE | (GPIO_CNF_INPUT_FLOATING << 2));
    }

    switch (function) {
    case HAL_GPIO_FUNCTION_GPIO:
        return GPIO_OUTPUT_MODE |
               (GPIO_CNF_OUTPUT_PUSHPULL << 2);
    case HAL_GPIO_FUNCTION_ALT_0:
        return GPIO_OUTPUT_MODE |
               (GPIO_CNF_OUTPUT_ALTERNATE_PUSHPULL << 2);
    case HAL_GPIO_FUNCTION_ALT_1:
        return GPIO_OUTPUT_MODE |
               (GPIO_CNF_OUTPUT_ALTERNATE_OPENDRAIN << 2);
    default:
        return 0xFFFFFFFFu;
    }
}

ecos_err_t hal_gpio_configure(hal_gpio_port_t port,
                              uint8_t pin,
                              const hal_gpio_config_t *config)
{
    uint32_t value;

    if (!valid_pin(port, pin) || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    value = output_config(config->direction, config->function);
    if (value == 0xFFFFFFFFu)
        return ECOS_ERR_UNSUPPORTED;

    enable_port_clock(port);
    write_config(port, pin, value);
    return ECOS_OK;
}

ecos_err_t hal_gpio_set_direction(hal_gpio_port_t port,
                                  uint8_t pin,
                                  hal_gpio_direction_t direction)
{
    uint32_t config;
    hal_gpio_function_t function;

    if (!valid_pin(port, pin) ||
        (direction != HAL_GPIO_DIRECTION_INPUT &&
         direction != HAL_GPIO_DIRECTION_OUTPUT))
        return ECOS_ERR_INVALID_ARGUMENT;

    enable_port_clock(port);
    config = read_config(port, pin);
    if (direction == HAL_GPIO_DIRECTION_INPUT ||
        (config & 0x3u) == GPIO_INPUT_MODE)
        function = HAL_GPIO_FUNCTION_GPIO;
    else if ((config >> 2) == GPIO_CNF_OUTPUT_ALTERNATE_PUSHPULL)
        function = HAL_GPIO_FUNCTION_ALT_0;
    else if ((config >> 2) == GPIO_CNF_OUTPUT_ALTERNATE_OPENDRAIN)
        function = HAL_GPIO_FUNCTION_ALT_1;
    else
        function = HAL_GPIO_FUNCTION_GPIO;
    config = output_config(direction, function);
    if (config == 0xFFFFFFFFu)
        return ECOS_ERR_UNSUPPORTED;

    write_config(port, pin, config);
    return ECOS_OK;
}

ecos_err_t hal_gpio_set_level(hal_gpio_port_t port,
                              uint8_t pin,
                              hal_gpio_level_t level)
{
    uint32_t bit;

    if (!valid_pin(port, pin) ||
        (level != HAL_GPIO_LEVEL_LOW && level != HAL_GPIO_LEVEL_HIGH))
        return ECOS_ERR_INVALID_ARGUMENT;

    enable_port_clock(port);
    bit = 1u << pin;
    if (level == HAL_GPIO_LEVEL_HIGH)
        *port_register(port, GPIO_BSHR_OFFSET) = bit;
    else
        *port_register(port, GPIO_BCR_OFFSET) = bit;
    return ECOS_OK;
}

int hal_gpio_get_level(hal_gpio_port_t port, uint8_t pin)
{
    if (!valid_pin(port, pin))
        return ECOS_ERR_INVALID_ARGUMENT;
    enable_port_clock(port);
    return (*port_register(port, GPIO_INDR_OFFSET) & (1u << pin)) != 0u ?
           HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW;
}

ecos_err_t hal_gpio_set_function(hal_gpio_port_t port,
                                 uint8_t pin,
                                 hal_gpio_function_t function)
{
    uint32_t current;
    uint32_t mode;
    uint32_t value;

    if (!valid_pin(port, pin) || function < HAL_GPIO_FUNCTION_GPIO ||
        function > HAL_GPIO_FUNCTION_ALT_1)
        return ECOS_ERR_INVALID_ARGUMENT;

    enable_port_clock(port);
    current = read_config(port, pin);
    mode = current & 0x3u;
    if (mode == GPIO_INPUT_MODE && function != HAL_GPIO_FUNCTION_GPIO)
        return ECOS_ERR_UNSUPPORTED;
    value = mode;
    if (function == HAL_GPIO_FUNCTION_GPIO)
        value |= (mode == GPIO_INPUT_MODE ? GPIO_CNF_INPUT_FLOATING :
                                            GPIO_CNF_OUTPUT_PUSHPULL) << 2;
    else if (function == HAL_GPIO_FUNCTION_ALT_0)
        value |= GPIO_CNF_OUTPUT_ALTERNATE_PUSHPULL << 2;
    else
        value |= GPIO_CNF_OUTPUT_ALTERNATE_OPENDRAIN << 2;

    write_config(port, pin, value);
    return ECOS_OK;
}
