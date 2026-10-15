#include "ecos/hal/rng.h"
#include "legacy_soc.h"

#include <stddef.h>

ecos_err_t hal_rng_init(hal_rng_id_t id, const hal_rng_config_t *config)
{
    if (id != HAL_RNG_ID_0 || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_RNG_0_SEED = config->seed;
    REG_RNG_0_CTRL = 1u;
    return ECOS_OK;
}

ecos_err_t hal_rng_deinit(hal_rng_id_t id)
{
    if (id != HAL_RNG_ID_0)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_RNG_0_CTRL = 0u;
    return ECOS_OK;
}

ecos_err_t hal_rng_read(hal_rng_id_t id, uint32_t *value)
{
    if (id != HAL_RNG_ID_0 || value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    *value = REG_RNG_0_VAL;
    return ECOS_OK;
}
