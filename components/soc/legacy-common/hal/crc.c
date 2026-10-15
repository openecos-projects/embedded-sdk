#include "ecos/hal/crc.h"
#include "legacy_soc.h"

#include <stddef.h>

#define CRC_CTRL_ENABLE 0x1u
#define CRC_CTRL_LOAD_INIT 0x8u
#define CRC_CTRL_MODE_SHIFT 5u

ecos_err_t hal_crc_init(hal_crc_id_t id, const hal_crc_config_t *config)
{
    if (id != HAL_CRC_ID_0 || config == NULL || config->mode > 3u)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_CRC_0_CTRL = 0u;
    REG_CRC_0_INIT = config->init_value;
    REG_CRC_0_XORV = config->xor_out;
    REG_CRC_0_CTRL = CRC_CTRL_ENABLE | CRC_CTRL_LOAD_INIT |
                     ((uint32_t)config->mode << CRC_CTRL_MODE_SHIFT);
    return ECOS_OK;
}

ecos_err_t hal_crc_deinit(hal_crc_id_t id)
{
    if (id != HAL_CRC_ID_0)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_CRC_0_CTRL = 0u;
    return ECOS_OK;
}

ecos_err_t hal_crc_feed(hal_crc_id_t id, uint32_t word)
{
    if (id != HAL_CRC_ID_0)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_CRC_0_DATA = word;
    return ECOS_OK;
}

ecos_err_t hal_crc_read(hal_crc_id_t id, uint32_t *result)
{
    if (id != HAL_CRC_ID_0 || result == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    *result = REG_CRC_0_DATA;
    return ECOS_OK;
}
