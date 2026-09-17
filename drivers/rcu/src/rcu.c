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
