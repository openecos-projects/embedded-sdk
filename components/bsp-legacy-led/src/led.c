#include "ecos/board_resources.h"
#include "ecos/bsp/led.h"
#include "ecos/driver/gpio.h"

#include <stdint.h>

#if !ECOS_BOARD_HAS_LED || ECOS_BOARD_LED_COUNT != 2u
#error "Legacy LED BSP requires two declared Board LEDs"
#endif

typedef struct {
    ecos_gpio_port_t port;
    uint8_t pin;
} led_gpio_t;

static const led_gpio_t led_gpios[BSP_LED_COUNT] = {
    [BSP_LED_0] = { ECOS_BOARD_LED_0_PORT, ECOS_BOARD_LED_0_PIN },
    [BSP_LED_1] = { ECOS_BOARD_LED_1_PORT, ECOS_BOARD_LED_1_PIN },
};

static uint8_t leds_initialized;

static ecos_err_t led_map_gpio_result(int result)
{
    if (result >= 0)
        return result;
    return ecos_err_is_known(result) ? result : ECOS_ERR_IO;
}

ecos_err_t bsp_led_init(void)
{
    const ecos_gpio_config_t config = {
        ECOS_GPIO_DIRECTION_OUTPUT,
        ECOS_GPIO_FUNCTION_GPIO,
    };
    const ecos_gpio_level_t off_level =
        ECOS_BOARD_LED_ACTIVE_LEVEL == ECOS_GPIO_LEVEL_HIGH ?
        ECOS_GPIO_LEVEL_LOW : ECOS_GPIO_LEVEL_HIGH;
    int index;

    leds_initialized = 0u;
    for (index = BSP_LED_0; index < BSP_LED_COUNT; ++index) {
        const led_gpio_t *gpio = &led_gpios[index];
        int result = ecos_gpio_set_level(gpio->port, gpio->pin, off_level);
        if (result != ECOS_OK)
            return led_map_gpio_result(result);
        result = ecos_gpio_configure(gpio->port, gpio->pin, &config);
        if (result != ECOS_OK)
            return led_map_gpio_result(result);
    }
    leds_initialized = 1u;
    return ECOS_OK;
}

ecos_err_t bsp_led_set_state(bsp_led_t led, bsp_led_state_t state)
{
    ecos_gpio_level_t level;

    if (led < BSP_LED_0 || led >= BSP_LED_COUNT ||
        (state != BSP_LED_OFF && state != BSP_LED_ON))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (leds_initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    level = state == BSP_LED_ON ? ECOS_BOARD_LED_ACTIVE_LEVEL :
        (ECOS_BOARD_LED_ACTIVE_LEVEL == ECOS_GPIO_LEVEL_HIGH ?
         ECOS_GPIO_LEVEL_LOW : ECOS_GPIO_LEVEL_HIGH);
    return led_map_gpio_result(
        ecos_gpio_set_level(led_gpios[led].port, led_gpios[led].pin, level)
    );
}
