#include "ecos/hal/rcu.h"
#include "ysyx_2512_1_soc.h"

#include <stddef.h>

static int rcu_id_is_valid(hal_rcu_id_t id)
{
    return id == HAL_RCU_ID_0;
}

ecos_err_t hal_rcu_init(hal_rcu_id_t id, const hal_rcu_config_t *config)
{
    if (!rcu_id_is_valid(id) || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (config->clock_divider == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    REG_RCU_0_RDIV = config->clock_divider - 1u;
    REG_RCU_0_CTRL = config->control;
    return ECOS_OK;
}

ecos_err_t hal_rcu_deinit(hal_rcu_id_t id)
{
    if (!rcu_id_is_valid(id))
        return ECOS_ERR_INVALID_ARGUMENT;

    REG_RCU_0_CTRL = 0u;
    return ECOS_OK;
}

ecos_err_t hal_rcu_get_status(hal_rcu_id_t id, uint32_t *status)
{
    if (!rcu_id_is_valid(id) || status == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    *status = REG_RCU_0_STAT;
    return ECOS_OK;
}
