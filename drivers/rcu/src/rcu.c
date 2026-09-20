#include "ecos/driver/rcu.h"

#include "ecos/hal/rcu.h"

#include <stddef.h>
#include <stdint.h>

static int rcu_map_hal_result(int result)
{
    if (result >= 0)
        return result;
    return ecos_err_is_known(result) ? result : ECOS_ERR_IO;
}

ecos_err_t ecos_rcu_init(ecos_rcu_id_t id, const ecos_rcu_config_t *config)
{
    hal_rcu_config_t hal_config;

    if (id >= ECOS_RCU_ID_COUNT || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (config->clock_divider == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    hal_config.clock_divider = config->clock_divider;
    hal_config.control = config->control;
    return rcu_map_hal_result(hal_rcu_init((hal_rcu_id_t)id, &hal_config));
}

ecos_err_t ecos_rcu_deinit(ecos_rcu_id_t id)
{
    if (id >= ECOS_RCU_ID_COUNT)
        return ECOS_ERR_INVALID_ARGUMENT;

    return rcu_map_hal_result(hal_rcu_deinit((hal_rcu_id_t)id));
}

ecos_err_t ecos_rcu_get_status(ecos_rcu_id_t id, uint32_t *status)
{
    if (id >= ECOS_RCU_ID_COUNT || status == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    return rcu_map_hal_result(hal_rcu_get_status((hal_rcu_id_t)id, status));
}

ecos_err_t ecos_rcu_pll_apply(ecos_rcu_id_t id,
                              const ecos_rcu_pll_config_t *config)
{
    hal_rcu_pll_config_t hal_config;

    if (id >= ECOS_RCU_ID_COUNT || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (config->n == 0u ||
        (unsigned)config->select > (unsigned)ECOS_RCU_PLL_SELECT_X2 ||
        (unsigned)config->od > (unsigned)ECOS_RCU_PLL_OD_8)
        return ECOS_ERR_INVALID_ARGUMENT;

    hal_config.n = config->n;
    hal_config.select = (hal_rcu_pll_select_t)config->select;
    hal_config.od = (hal_rcu_pll_od_t)config->od;
    hal_config.cpu_div = config->cpu_div;
    hal_config.axi_div = config->axi_div;
    hal_config.ahb_div = config->ahb_div;
    hal_config.keep_apb = config->keep_apb;
    hal_config.settle_ticks = config->settle_ticks;
    return rcu_map_hal_result(hal_rcu_pll_apply((hal_rcu_id_t)id, &hal_config));
}

ecos_err_t ecos_rcu_pll_get_freq(ecos_rcu_id_t id,
                                 ecos_rcu_clock_domain_t domain,
                                 uint32_t *freq_hz)
{
    if (id >= ECOS_RCU_ID_COUNT || freq_hz == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if ((unsigned)domain > (unsigned)ECOS_RCU_CLOCK_APB)
        return ECOS_ERR_INVALID_ARGUMENT;

    return rcu_map_hal_result(
        hal_rcu_pll_get_freq(
            (hal_rcu_id_t)id, (hal_rcu_clock_domain_t)domain, freq_hz
        )
    );
}
