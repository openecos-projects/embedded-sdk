#include "ecos/driver/gpio.h"

#include "ecos/hal/gpio.h"

#include <stddef.h>

static int gpio_port_is_valid(ecos_gpio_port_t port)
{
    return port >= ECOS_GPIO_PORT_0 && port < ECOS_GPIO_PORT_COUNT;
}

static int gpio_pin_is_valid(uint8_t pin)
{
    return pin < 32u;
}

static int gpio_direction_is_valid(ecos_gpio_direction_t direction)
{
    return direction == ECOS_GPIO_DIRECTION_INPUT ||
           direction == ECOS_GPIO_DIRECTION_OUTPUT;
}

static int gpio_level_is_valid(ecos_gpio_level_t level)
{
    return level == ECOS_GPIO_LEVEL_LOW || level == ECOS_GPIO_LEVEL_HIGH;
}

static int gpio_function_is_valid(ecos_gpio_function_t function)
{
    return function >= ECOS_GPIO_FUNCTION_GPIO &&
           function <= ECOS_GPIO_FUNCTION_ALT_1;
}

static int gpio_map_hal_result(int result)
{
    if (result >= 0)
        return result;
    return ecos_err_is_known(result) ? result : ECOS_ERR_IO;
}

ecos_err_t ecos_gpio_configure(ecos_gpio_port_t port,
                               uint8_t pin,
                               const ecos_gpio_config_t *config)
{
    hal_gpio_config_t hal_config;

    if (!gpio_port_is_valid(port) || !gpio_pin_is_valid(pin) || config == NULL ||
        !gpio_direction_is_valid(config->direction) ||
        !gpio_function_is_valid(config->function))
        return ECOS_ERR_INVALID_ARGUMENT;

    hal_config.direction = (hal_gpio_direction_t)config->direction;
    hal_config.function = (hal_gpio_function_t)config->function;
    return gpio_map_hal_result(
        hal_gpio_configure((hal_gpio_port_t)port, pin, &hal_config)
    );
}

ecos_err_t ecos_gpio_set_direction(ecos_gpio_port_t port,
                                   uint8_t pin,
                                   ecos_gpio_direction_t direction)
{
    if (!gpio_port_is_valid(port) || !gpio_pin_is_valid(pin) ||
        !gpio_direction_is_valid(direction))
        return ECOS_ERR_INVALID_ARGUMENT;

    return gpio_map_hal_result(
        hal_gpio_set_direction(
            (hal_gpio_port_t)port, pin, (hal_gpio_direction_t)direction
        )
    );
}

ecos_err_t ecos_gpio_set_level(ecos_gpio_port_t port,
                               uint8_t pin,
                               ecos_gpio_level_t level)
{
    if (!gpio_port_is_valid(port) || !gpio_pin_is_valid(pin) ||
        !gpio_level_is_valid(level))
        return ECOS_ERR_INVALID_ARGUMENT;

    return gpio_map_hal_result(
        hal_gpio_set_level((hal_gpio_port_t)port, pin, (hal_gpio_level_t)level)
    );
}

int ecos_gpio_get_level(ecos_gpio_port_t port, uint8_t pin)
{
    if (!gpio_port_is_valid(port) || !gpio_pin_is_valid(pin))
        return ECOS_ERR_INVALID_ARGUMENT;

    return gpio_map_hal_result(hal_gpio_get_level((hal_gpio_port_t)port, pin));
}

ecos_err_t ecos_gpio_set_function(ecos_gpio_port_t port,
                                  uint8_t pin,
                                  ecos_gpio_function_t function)
{
    if (!gpio_port_is_valid(port) || !gpio_pin_is_valid(pin) ||
        !gpio_function_is_valid(function))
        return ECOS_ERR_INVALID_ARGUMENT;

    return gpio_map_hal_result(
        hal_gpio_set_function(
            (hal_gpio_port_t)port, pin, (hal_gpio_function_t)function
        )
    );
}
