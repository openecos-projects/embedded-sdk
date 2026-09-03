#include "ecos/hal/gpio.h"
#include "ysyx_2512_soc.h"

#include <stddef.h>

static int gpio_port_is_valid(hal_gpio_port_t port)
{
    return port >= HAL_GPIO_PORT_0 && port < HAL_GPIO_PORT_COUNT;
}

static int gpio_pin_is_valid(uint8_t pin)
{
    return pin < 32u;
}

static volatile uint32_t *gpio_direction_register(hal_gpio_port_t port)
{
    switch (port) {
    case HAL_GPIO_PORT_0:
        return &REG_GPIO_0_PADDIR;
    case HAL_GPIO_PORT_1:
        return &REG_GPIO_1_PADDIR;
    case HAL_GPIO_PORT_2:
        return &REG_GPIO_2_PADDIR;
    default:
        return NULL;
    }
}

static volatile uint32_t *gpio_input_register(hal_gpio_port_t port)
{
    switch (port) {
    case HAL_GPIO_PORT_0:
        return &REG_GPIO_0_PADIN;
    case HAL_GPIO_PORT_1:
        return &REG_GPIO_1_PADIN;
    case HAL_GPIO_PORT_2:
        return &REG_GPIO_2_PADIN;
    default:
        return NULL;
    }
}

static volatile uint32_t *gpio_output_register(hal_gpio_port_t port)
{
    switch (port) {
    case HAL_GPIO_PORT_0:
        return &REG_GPIO_0_PADOUT;
    case HAL_GPIO_PORT_1:
        return &REG_GPIO_1_PADOUT;
    case HAL_GPIO_PORT_2:
        return &REG_GPIO_2_PADOUT;
    default:
        return NULL;
    }
}

static volatile uint32_t *gpio_function_register(hal_gpio_port_t port)
{
    switch (port) {
    case HAL_GPIO_PORT_0:
        return &REG_GPIO_0_IOFCFG;
    case HAL_GPIO_PORT_1:
        return &REG_GPIO_1_IOFCFG;
    case HAL_GPIO_PORT_2:
        return &REG_GPIO_2_IOFCFG;
    default:
        return NULL;
    }
}

static volatile uint32_t *gpio_mux_register(hal_gpio_port_t port)
{
    switch (port) {
    case HAL_GPIO_PORT_0:
        return &REG_GPIO_0_PINMUX;
    case HAL_GPIO_PORT_1:
        return &REG_GPIO_1_PINMUX;
    case HAL_GPIO_PORT_2:
        return &REG_GPIO_2_PINMUX;
    default:
        return NULL;
    }
}

int hal_gpio_set_direction(hal_gpio_port_t port,
                           uint8_t pin,
                           hal_gpio_direction_t direction)
{
    volatile uint32_t *direction_register;
    uint32_t bit;

    if (!gpio_port_is_valid(port) || !gpio_pin_is_valid(pin) ||
        (direction != HAL_GPIO_DIRECTION_INPUT &&
         direction != HAL_GPIO_DIRECTION_OUTPUT))
        return HAL_GPIO_ERROR_INVALID_ARGUMENT;

    direction_register = gpio_direction_register(port);
    bit = (uint32_t)1u << pin;
    if (direction == HAL_GPIO_DIRECTION_OUTPUT)
        *direction_register |= bit;
    else
        *direction_register &= ~bit;
    return HAL_GPIO_OK;
}

int hal_gpio_set_level(hal_gpio_port_t port,
                       uint8_t pin,
                       hal_gpio_level_t level)
{
    volatile uint32_t *output_register;
    uint32_t bit;

    if (!gpio_port_is_valid(port) || !gpio_pin_is_valid(pin) ||
        (level != HAL_GPIO_LEVEL_LOW && level != HAL_GPIO_LEVEL_HIGH))
        return HAL_GPIO_ERROR_INVALID_ARGUMENT;

    output_register = gpio_output_register(port);
    bit = (uint32_t)1u << pin;
    if (level == HAL_GPIO_LEVEL_HIGH)
        *output_register |= bit;
    else
        *output_register &= ~bit;
    return HAL_GPIO_OK;
}

int hal_gpio_get_level(hal_gpio_port_t port, uint8_t pin)
{
    volatile uint32_t *input_register;
    uint32_t bit;

    if (!gpio_port_is_valid(port) || !gpio_pin_is_valid(pin))
        return HAL_GPIO_ERROR_INVALID_ARGUMENT;

    input_register = gpio_input_register(port);
    bit = (uint32_t)1u << pin;
    return (*input_register & bit) != 0u ?
           HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW;
}

int hal_gpio_set_function(hal_gpio_port_t port,
                          uint8_t pin,
                          hal_gpio_function_t function)
{
    volatile uint32_t *function_register;
    volatile uint32_t *mux_register;
    uint32_t bit;

    if (!gpio_port_is_valid(port) || !gpio_pin_is_valid(pin) ||
        function < HAL_GPIO_FUNCTION_GPIO ||
        function > HAL_GPIO_FUNCTION_ALT_1)
        return HAL_GPIO_ERROR_INVALID_ARGUMENT;

    function_register = gpio_function_register(port);
    mux_register = gpio_mux_register(port);
    bit = (uint32_t)1u << pin;
    if (function == HAL_GPIO_FUNCTION_GPIO) {
        *function_register &= ~bit;
    } else {
        if (function == HAL_GPIO_FUNCTION_ALT_1)
            *mux_register |= bit;
        else
            *mux_register &= ~bit;
        *function_register |= bit;
    }
    return HAL_GPIO_OK;
}

int hal_gpio_configure(hal_gpio_port_t port,
                       uint8_t pin,
                       const hal_gpio_config_t *config)
{
    int result;

    if (config == NULL ||
        (config->direction != HAL_GPIO_DIRECTION_INPUT &&
         config->direction != HAL_GPIO_DIRECTION_OUTPUT) ||
        config->function < HAL_GPIO_FUNCTION_GPIO ||
        config->function > HAL_GPIO_FUNCTION_ALT_1)
        return HAL_GPIO_ERROR_INVALID_ARGUMENT;

    result = hal_gpio_set_direction(port, pin, config->direction);
    if (result != HAL_GPIO_OK)
        return result;
    return hal_gpio_set_function(port, pin, config->function);
}
