#include "ecos/hal/rng.h"
#include "hal_rng.h"
#include "ysyx_2512_1_soc.h"

#include <stddef.h>

#define RNG_CTRL_ENABLE     0x1u

static int rng_id_is_valid(hal_rng_id_t id)
{
    return id == HAL_RNG_ID_0;
}

ecos_err_t hal_rng_init(hal_rng_id_t id, const hal_rng_config_t *config)
{
    if (!rng_id_is_valid(id) || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    REG_RNG_0_SEED = config->seed;
    REG_RNG_0_CTRL = RNG_CTRL_ENABLE;
    return ECOS_OK;
}

ecos_err_t hal_rng_deinit(hal_rng_id_t id)
{
    if (!rng_id_is_valid(id))
        return ECOS_ERR_INVALID_ARGUMENT;

    REG_RNG_0_CTRL = 0u;
    return ECOS_OK;
}

ecos_err_t hal_rng_read(hal_rng_id_t id, uint32_t *value)
{
    if (!rng_id_is_valid(id) || value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    *value = REG_RNG_0_VAL;
    return ECOS_OK;
}

/* ---- SDK 2.x compatibility API ---- */

void hal_rng_set_ctrl(uint32_t val) {
    REG_RNG_0_CTRL = val;
}

void hal_rng_set_seed(uint32_t val) {
    REG_RNG_0_SEED = val;
}

uint32_t hal_rng_get_val(void) {
    return REG_RNG_0_VAL;
}
