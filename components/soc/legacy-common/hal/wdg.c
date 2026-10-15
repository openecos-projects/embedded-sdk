#include "ecos/hal/wdg.h"
#include "legacy_soc.h"

#include <stddef.h>

#define WDG_UNLOCK_KEY 0x5F3759DFu

ecos_err_t hal_wdg_init(hal_wdg_id_t id, const hal_wdg_config_t *config)
{
    if (id != HAL_WDG_ID_0 || config == NULL || config->prescaler == 0u ||
        config->compare == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_WDG_0_KEY = WDG_UNLOCK_KEY;
    REG_WDG_0_CTRL = 0u;
    REG_WDG_0_KEY = WDG_UNLOCK_KEY;
    REG_WDG_0_PSCR = config->prescaler - 1u;
    REG_WDG_0_KEY = WDG_UNLOCK_KEY;
    REG_WDG_0_CMP = config->compare - 1u;
    REG_WDG_0_KEY = WDG_UNLOCK_KEY;
    REG_WDG_0_CTRL = config->control;
    return ECOS_OK;
}

ecos_err_t hal_wdg_deinit(hal_wdg_id_t id)
{
    if (id != HAL_WDG_ID_0)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_WDG_0_KEY = WDG_UNLOCK_KEY;
    REG_WDG_0_CTRL = 0u;
    return ECOS_OK;
}

ecos_err_t hal_wdg_feed(hal_wdg_id_t id)
{
    if (id != HAL_WDG_ID_0)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_WDG_0_KEY = WDG_UNLOCK_KEY;
    REG_WDG_0_FEED = 1u;
    REG_WDG_0_KEY = WDG_UNLOCK_KEY;
    REG_WDG_0_FEED = 0u;
    return ECOS_OK;
}

ecos_err_t hal_wdg_get_status(hal_wdg_id_t id, uint32_t *status)
{
    if (id != HAL_WDG_ID_0 || status == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    *status = REG_WDG_0_STAT;
    return ECOS_OK;
}
