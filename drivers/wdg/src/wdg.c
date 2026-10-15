#include "ecos/driver/wdg.h"

#include "ecos/hal/wdg.h"

#include <stddef.h>

static int wdg_map_hal_result(int result)
{
    if (result >= 0)
        return result;
    return ecos_err_is_known(result) ? result : ECOS_ERR_IO;
}

ecos_err_t ecos_wdg_init(ecos_wdg_id_t id, const ecos_wdg_config_t *config)
{
    hal_wdg_config_t hal_config;
    if (id >= ECOS_WDG_ID_COUNT || config == NULL || config->prescaler == 0u ||
        config->compare == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    hal_config.prescaler = config->prescaler;
    hal_config.compare = config->compare;
    hal_config.control = config->control;
    return wdg_map_hal_result(hal_wdg_init((hal_wdg_id_t)id, &hal_config));
}

ecos_err_t ecos_wdg_deinit(ecos_wdg_id_t id)
{
    if (id >= ECOS_WDG_ID_COUNT)
        return ECOS_ERR_INVALID_ARGUMENT;
    return wdg_map_hal_result(hal_wdg_deinit((hal_wdg_id_t)id));
}

ecos_err_t ecos_wdg_feed(ecos_wdg_id_t id)
{
    if (id >= ECOS_WDG_ID_COUNT)
        return ECOS_ERR_INVALID_ARGUMENT;
    return wdg_map_hal_result(hal_wdg_feed((hal_wdg_id_t)id));
}

ecos_err_t ecos_wdg_get_status(ecos_wdg_id_t id, uint32_t *status)
{
    if (id >= ECOS_WDG_ID_COUNT || status == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    return wdg_map_hal_result(
        hal_wdg_get_status((hal_wdg_id_t)id, status)
    );
}
