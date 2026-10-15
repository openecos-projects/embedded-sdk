#include "ecos/hal/ps2.h"
#include "ecos/hal/gpio.h"
#include "legacy_soc.h"

#include <stddef.h>

ecos_err_t hal_ps2_init(hal_ps2_id_t id)
{
    ecos_err_t result;
    if (id != HAL_PS2_ID_0)
        return ECOS_ERR_INVALID_ARGUMENT;

    result = hal_gpio_set_function(HAL_GPIO_PORT_1, 12u, HAL_GPIO_FUNCTION_ALT_0);
    if (result != ECOS_OK)
        return result;
    result = hal_gpio_set_function(HAL_GPIO_PORT_1, 13u, HAL_GPIO_FUNCTION_ALT_0);
    if (result != ECOS_OK)
        return result;
    REG_PS2_0_CTRL = 3u;
    return ECOS_OK;
}

ecos_err_t hal_ps2_deinit(hal_ps2_id_t id)
{
    if (id != HAL_PS2_ID_0)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_PS2_0_CTRL = 0u;
    return ECOS_OK;
}

ecos_err_t hal_ps2_read(hal_ps2_id_t id, uint32_t *scan_code)
{
    if (id != HAL_PS2_ID_0 || scan_code == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    *scan_code = REG_PS2_0_DATA;
    return ECOS_OK;
}
