#include "ecos/bsp/button.h"

#include "ecos/driver/gpio.h"

#include <stdint.h>

typedef struct {
    ecos_gpio_port_t port;
    uint8_t pin;
} button_gpio_t;

static const button_gpio_t button_gpios[BSP_BUTTON_COUNT] = {
    [BSP_BUTTON_0] = { ECOS_GPIO_PORT_1, 7u },
    [BSP_BUTTON_1] = { ECOS_GPIO_PORT_1, 8u },
};

static uint8_t buttons_initialized;

static int button_is_valid(bsp_button_t button)
{
    return button >= BSP_BUTTON_0 && button < BSP_BUTTON_COUNT;
}

static int button_map_gpio_result(int result)
{
    if (result >= 0)
        return result;
    return ecos_err_is_known(result) ? result : ECOS_ERR_IO;
}

ecos_err_t bsp_button_init(void)
{
    const ecos_gpio_config_t config = {
        ECOS_GPIO_DIRECTION_INPUT,
        ECOS_GPIO_FUNCTION_GPIO,
    };
    int button;

    buttons_initialized = 0u;
    for (button = BSP_BUTTON_0; button < BSP_BUTTON_COUNT; ++button) {
        const button_gpio_t *gpio = &button_gpios[button];
        int result = ecos_gpio_configure(gpio->port, gpio->pin, &config);

        if (result != ECOS_OK)
            return button_map_gpio_result(result);
    }
    buttons_initialized = 1u;
    return ECOS_OK;
}

int bsp_button_get_state(bsp_button_t button)
{
    const button_gpio_t *gpio;
    int level;

    if (!button_is_valid(button))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (buttons_initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;

    gpio = &button_gpios[button];
    level = button_map_gpio_result(ecos_gpio_get_level(gpio->port, gpio->pin));
    if (level < 0)
        return level;
    return level == ECOS_GPIO_LEVEL_LOW ?
           BSP_BUTTON_PRESSED : BSP_BUTTON_RELEASED;
}
