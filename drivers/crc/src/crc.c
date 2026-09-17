#include "ecos/driver/crc.h"

#include "ecos/hal/crc.h"

#include <stddef.h>
#include <stdint.h>

static int crc_map_hal_result(int result)
{
    if (result >= 0)
        return result;
    return ecos_err_is_known(result) ? result : ECOS_ERR_IO;
}

ecos_err_t ecos_crc_init(ecos_crc_id_t id, const ecos_crc_config_t *config)
{
    hal_crc_config_t hal_config;

    if (id >= ECOS_CRC_ID_COUNT || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    hal_config.init_value = config->init_value;
    hal_config.xor_out = config->xor_out;
    hal_config.mode = config->mode;
    return crc_map_hal_result(hal_crc_init((hal_crc_id_t)id, &hal_config));
}

ecos_err_t ecos_crc_deinit(ecos_crc_id_t id)
{
    if (id >= ECOS_CRC_ID_COUNT)
        return ECOS_ERR_INVALID_ARGUMENT;

    return crc_map_hal_result(hal_crc_deinit((hal_crc_id_t)id));
}

ecos_err_t ecos_crc_feed(ecos_crc_id_t id, uint32_t word)
{
    if (id >= ECOS_CRC_ID_COUNT)
        return ECOS_ERR_INVALID_ARGUMENT;

    return crc_map_hal_result(hal_crc_feed((hal_crc_id_t)id, word));
}

ecos_err_t ecos_crc_read(ecos_crc_id_t id, uint32_t *result)
{
    if (id >= ECOS_CRC_ID_COUNT || result == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    return crc_map_hal_result(hal_crc_read((hal_crc_id_t)id, result));
}

ecos_err_t ecos_crc_compute(ecos_crc_id_t id, const uint32_t *words,
                            size_t count, uint32_t *result)
{
    size_t index;

    if ((words == NULL && count != 0u) || result == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    for (index = 0u; index < count; ++index) {
        ecos_err_t err = ecos_crc_feed(id, words[index]);

        if (err != ECOS_OK)
            return err;
    }
    return ecos_crc_read(id, result);
}
