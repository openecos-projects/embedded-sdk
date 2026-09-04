#include "ecos/driver/i2c.h"

#include "ecos/hal/i2c.h"

#include <stddef.h>

static int i2c_map_hal_result(int result)
{
    if (result >= 0)
        return result;
    return ecos_err_is_known(result) ? result : ECOS_ERR_IO;
}

int ecos_i2c_get_instance_count(void)
{
    return i2c_map_hal_result(hal_i2c_get_instance_count());
}

ecos_err_t ecos_i2c_init(ecos_i2c_id_t i2c,
                         const ecos_i2c_config_t *config)
{
    hal_i2c_config_t hal_config;

    if (config == NULL || config->clock_divider == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    hal_config.clock_divider = config->clock_divider;
    return i2c_map_hal_result(
        hal_i2c_init((hal_i2c_id_t)i2c, &hal_config)
    );
}

ecos_err_t ecos_i2c_deinit(ecos_i2c_id_t i2c)
{
    return i2c_map_hal_result(hal_i2c_deinit((hal_i2c_id_t)i2c));
}

int ecos_i2c_probe(ecos_i2c_id_t i2c, uint8_t address)
{
    if (address > 0x7fu)
        return ECOS_ERR_INVALID_ARGUMENT;
    return i2c_map_hal_result(
        hal_i2c_probe((hal_i2c_id_t)i2c, address)
    );
}
