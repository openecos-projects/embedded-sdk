#include "ecos/bsp/led.h"

#include "ecos/driver/gpio.h"

#include <stdint.h>

typedef struct {
    ecos_gpio_port_t port;
    uint8_t pin;
} led_gpio_t;

static const led_gpio_t led_gpios[BSP_LED_COUNT] = {
    [BSP_LED_0] = { ECOS_GPIO_PORT_1, 5u },
    [BSP_LED_1] = { ECOS_GPIO_PORT_1, 6u },
};

static uint8_t leds_initialized;

static int led_is_valid(bsp_led_t led)
{
    return led >= BSP_LED_0 && led < BSP_LED_COUNT;
}

static int led_state_is_valid(bsp_led_state_t state)
{
    return state == BSP_LED_OFF || state == BSP_LED_ON;
}

static int led_map_gpio_result(int result)
{
    if (result >= 0)
        return result;
    if (result == ECOS_GPIO_ERROR_INVALID_ARGUMENT)
        return BSP_LED_ERROR_INVALID_ARGUMENT;
    if (result == ECOS_GPIO_ERROR_UNSUPPORTED)
        return BSP_LED_ERROR_UNSUPPORTED;
    return BSP_LED_ERROR_IO;
}

int bsp_led_init(void)
{
    const ecos_gpio_config_t config = {
        ECOS_GPIO_DIRECTION_OUTPUT,
        ECOS_GPIO_FUNCTION_GPIO,
    };
    int led;

    leds_initialized = 0u;
    for (led = BSP_LED_0; led < BSP_LED_COUNT; ++led) {
        const led_gpio_t *gpio = &led_gpios[led];
        int result = ecos_gpio_set_level(
            gpio->port, gpio->pin, ECOS_GPIO_LEVEL_HIGH
        );

        if (result != ECOS_GPIO_OK)
            return led_map_gpio_result(result);
        result = ecos_gpio_configure(gpio->port, gpio->pin, &config);
        if (result != ECOS_GPIO_OK)
            return led_map_gpio_result(result);
    }
    leds_initialized = 1u;
    return BSP_LED_OK;
}

int bsp_led_set_state(bsp_led_t led, bsp_led_state_t state)
{
    const led_gpio_t *gpio;
    ecos_gpio_level_t level;

    if (!led_is_valid(led) || !led_state_is_valid(state))
        return BSP_LED_ERROR_INVALID_ARGUMENT;
    if (leds_initialized == 0u)
        return BSP_LED_ERROR_NOT_INITIALIZED;

    gpio = &led_gpios[led];
    level = state == BSP_LED_ON ?
            ECOS_GPIO_LEVEL_LOW : ECOS_GPIO_LEVEL_HIGH;
    return led_map_gpio_result(
        ecos_gpio_set_level(gpio->port, gpio->pin, level)
    );
}
