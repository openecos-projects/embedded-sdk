#include "ecos/bsp/button.h"

#include "ecos/driver/gpio.h"

#include <stdint.h>

static uint8_t button_initialized;

ecos_err_t bsp_button_init(void)
{
    const ecos_gpio_config_t config = {
        ECOS_GPIO_DIRECTION_INPUT,
        ECOS_GPIO_FUNCTION_GPIO,
    };
    int result;

    button_initialized = 0u;
    result = ecos_gpio_configure(ECOS_GPIO_PORT_0, 7u, &config);
    if (result != ECOS_OK)
        return ecos_err_is_known(result) ? result : ECOS_ERR_IO;
    button_initialized = 1u;
    return ECOS_OK;
}

int bsp_button_get_count(void)
{
    return 1;
}

int bsp_button_get_state(bsp_button_t button)
{
    int level;

    if (button != BSP_BUTTON_0)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (button_initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    level = ecos_gpio_get_level(ECOS_GPIO_PORT_0, 7u);
    if (level < 0)
        return ecos_err_is_known(level) ? level : ECOS_ERR_IO;
    return level == ECOS_GPIO_LEVEL_LOW ?
           BSP_BUTTON_PRESSED : BSP_BUTTON_RELEASED;
}
