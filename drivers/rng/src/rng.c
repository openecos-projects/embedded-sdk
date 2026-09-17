#include "ecos/driver/rng.h"

#include "ecos/hal/rng.h"

#include <stddef.h>
#include <stdint.h>

static int rng_map_hal_result(int result)
{
    if (result >= 0)
        return result;
    return ecos_err_is_known(result) ? result : ECOS_ERR_IO;
}

ecos_err_t ecos_rng_init(ecos_rng_id_t id, const ecos_rng_config_t *config)
{
    hal_rng_config_t hal_config;

    if (id >= ECOS_RNG_ID_COUNT || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    hal_config.seed = config->seed;
    return rng_map_hal_result(hal_rng_init((hal_rng_id_t)id, &hal_config));
}

ecos_err_t ecos_rng_deinit(ecos_rng_id_t id)
{
    if (id >= ECOS_RNG_ID_COUNT)
        return ECOS_ERR_INVALID_ARGUMENT;

    return rng_map_hal_result(hal_rng_deinit((hal_rng_id_t)id));
}

ecos_err_t ecos_rng_read(ecos_rng_id_t id, uint32_t *value)
{
    if (id >= ECOS_RNG_ID_COUNT || value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    return rng_map_hal_result(hal_rng_read((hal_rng_id_t)id, value));
}
