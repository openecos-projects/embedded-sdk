#include "ecos/hal/crc.h"
#include "hal_crc.h"
#include "ysyx_2512_2_soc.h"

#include <stddef.h>

#define CRC_CTRL_ENABLE         ((uint32_t)0x1u << 0)
#define CRC_CTRL_LOAD_INIT      ((uint32_t)0x1u << 3)
#define CRC_CTRL_MODE_SHIFT     5u
#define CRC_MODE_MAX            3u

static int crc_id_is_valid(hal_crc_id_t id)
{
    return id == HAL_CRC_ID_0;
}

ecos_err_t hal_crc_init(hal_crc_id_t id, const hal_crc_config_t *config)
{
    if (!crc_id_is_valid(id) || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (config->mode > CRC_MODE_MAX)
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
    if (!crc_id_is_valid(id))
        return ECOS_ERR_INVALID_ARGUMENT;

    REG_CRC_0_CTRL = 0u;
    return ECOS_OK;
}

ecos_err_t hal_crc_feed(hal_crc_id_t id, uint32_t word)
{
    if (!crc_id_is_valid(id))
        return ECOS_ERR_INVALID_ARGUMENT;

    REG_CRC_0_DATA = word;
    return ECOS_OK;
}

ecos_err_t hal_crc_read(hal_crc_id_t id, uint32_t *result)
{
    if (!crc_id_is_valid(id) || result == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    *result = REG_CRC_0_DATA;
    return ECOS_OK;
}

/* ---- SDK 2.x compatibility API ---- */

void hal_crc_set_ctrl(uint32_t val) {
    REG_CRC_0_CTRL = val;
}

void hal_crc_set_init(uint32_t val) {
    REG_CRC_0_INIT = val;
}

void hal_crc_set_xorv(uint32_t val) {
    REG_CRC_0_XORV = val;
}

void hal_crc_set_data(uint32_t val) {
    REG_CRC_0_DATA = val;
}

uint32_t hal_crc_get_val(void) {
    return REG_CRC_0_DATA;
}
