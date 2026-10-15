#include "ecos/hal/rcu.h"
#include "legacy_soc.h"

#include <stddef.h>

ecos_err_t hal_rcu_init(hal_rcu_id_t id, const hal_rcu_config_t *config)
{
    if (id != HAL_RCU_ID_0 || config == NULL || config->clock_divider == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_RCU_0_RDIV = config->clock_divider - 1u;
    REG_RCU_0_CTRL = config->control;
    return ECOS_OK;
}

ecos_err_t hal_rcu_deinit(hal_rcu_id_t id)
{
    if (id != HAL_RCU_ID_0)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_RCU_0_CTRL = 0u;
    return ECOS_OK;
}

ecos_err_t hal_rcu_get_status(hal_rcu_id_t id, uint32_t *status)
{
    if (id != HAL_RCU_ID_0 || status == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    *status = REG_RCU_0_STAT;
    return ECOS_OK;
}

ecos_err_t hal_rcu_pll_apply(hal_rcu_id_t id,
                             const hal_rcu_pll_config_t *config)
{
    (void)id;
    (void)config;
    return ECOS_ERR_UNSUPPORTED;
}

ecos_err_t hal_rcu_pll_get_freq(hal_rcu_id_t id,
                                hal_rcu_clock_domain_t domain,
                                uint32_t *freq_hz)
{
    (void)id;
    (void)domain;
    (void)freq_hz;
    return ECOS_ERR_UNSUPPORTED;
}
