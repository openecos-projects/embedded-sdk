#include "ecos/hal/gpio.h"
#include "cl1_2512_soc.h"

#include <stddef.h>

static volatile uint32_t *const gpio_output_registers[CL1_2512_GPIO_PORT_COUNT] = {
    &REG_GPIO_0_SWPORTA_DR,
    &REG_GPIO_0_SWPORTB_DR,
    &REG_GPIO_0_SWPORTC_DR,
    &REG_GPIO_0_SWPORTD_DR,
};

static volatile uint32_t *const gpio_direction_registers[CL1_2512_GPIO_PORT_COUNT] = {
    &REG_GPIO_0_SWPORTA_DDR,
    &REG_GPIO_0_SWPORTB_DDR,
    &REG_GPIO_0_SWPORTC_DDR,
    &REG_GPIO_0_SWPORTD_DDR,
};

static volatile uint32_t *const gpio_input_registers[CL1_2512_GPIO_PORT_COUNT] = {
    &REG_GPIO_0_EXT_PORTA,
    &REG_GPIO_0_EXT_PORTB,
    &REG_GPIO_0_EXT_PORTC,
    &REG_GPIO_0_EXT_PORTD,
};

static int gpio_port_is_valid(hal_gpio_port_t port)
{
    return port >= HAL_GPIO_PORT_0 && port <= HAL_GPIO_PORT_3;
}

static int gpio_pin_is_valid(uint8_t pin)
{
    return pin < CL1_2512_GPIO_PINS_PER_PORT;
}

ecos_err_t hal_gpio_set_direction(hal_gpio_port_t port,
                                  uint8_t pin,
                                  hal_gpio_direction_t direction)
{
    volatile uint32_t *direction_register;
    uint32_t bit;

    if (!gpio_port_is_valid(port) || !gpio_pin_is_valid(pin) ||
        (direction != HAL_GPIO_DIRECTION_INPUT &&
         direction != HAL_GPIO_DIRECTION_OUTPUT))
        return ECOS_ERR_INVALID_ARGUMENT;

    direction_register = gpio_direction_registers[port];
    bit = (uint32_t)1u << pin;
    if (direction == HAL_GPIO_DIRECTION_OUTPUT)
        *direction_register |= bit;
    else
        *direction_register &= ~bit;
    return ECOS_OK;
}

ecos_err_t hal_gpio_set_level(hal_gpio_port_t port,
                              uint8_t pin,
                              hal_gpio_level_t level)
{
    volatile uint32_t *output_register;
    uint32_t bit;

    if (!gpio_port_is_valid(port) || !gpio_pin_is_valid(pin) ||
        (level != HAL_GPIO_LEVEL_LOW && level != HAL_GPIO_LEVEL_HIGH))
        return ECOS_ERR_INVALID_ARGUMENT;

    output_register = gpio_output_registers[port];
    bit = (uint32_t)1u << pin;
    if (level == HAL_GPIO_LEVEL_HIGH)
        *output_register |= bit;
    else
        *output_register &= ~bit;
    return ECOS_OK;
}

int hal_gpio_get_level(hal_gpio_port_t port, uint8_t pin)
{
    uint32_t bit;

    if (!gpio_port_is_valid(port) || !gpio_pin_is_valid(pin))
        return ECOS_ERR_INVALID_ARGUMENT;

    bit = (uint32_t)1u << pin;
    return (*gpio_input_registers[port] & bit) != 0u ?
           HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW;
}

ecos_err_t hal_gpio_set_function(hal_gpio_port_t port,
                                 uint8_t pin,
                                 hal_gpio_function_t function)
{
    if (!gpio_port_is_valid(port) || !gpio_pin_is_valid(pin) ||
        function < HAL_GPIO_FUNCTION_GPIO ||
        function > HAL_GPIO_FUNCTION_ALT_1)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (function != HAL_GPIO_FUNCTION_GPIO)
        return ECOS_ERR_UNSUPPORTED;
    return ECOS_OK;
}

ecos_err_t hal_gpio_configure(hal_gpio_port_t port,
                              uint8_t pin,
                              const hal_gpio_config_t *config)
{
    int result;

    if (config == NULL ||
        (config->direction != HAL_GPIO_DIRECTION_INPUT &&
         config->direction != HAL_GPIO_DIRECTION_OUTPUT) ||
        config->function < HAL_GPIO_FUNCTION_GPIO ||
        config->function > HAL_GPIO_FUNCTION_ALT_1)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (config->function != HAL_GPIO_FUNCTION_GPIO)
        return ECOS_ERR_UNSUPPORTED;

    result = hal_gpio_set_direction(port, pin, config->direction);
    if (result != ECOS_OK)
        return result;
    return hal_gpio_set_function(port, pin, config->function);
}
