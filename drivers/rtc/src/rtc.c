#include "ecos/driver/rtc.h"

#include "ecos/hal/rtc.h"

#include <stddef.h>
#include <stdint.h>

static int rtc_map_hal_result(int result)
{
    if (result >= 0)
        return result;
    return ecos_err_is_known(result) ? result : ECOS_ERR_IO;
}

ecos_err_t ecos_rtc_init(ecos_rtc_id_t id, const ecos_rtc_config_t *config)
{
    hal_rtc_config_t hal_config;

    if (id >= ECOS_RTC_ID_COUNT || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (config->prescaler == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    hal_config.prescaler = config->prescaler;
    hal_config.alarm_enabled = config->alarm_enabled;
    return rtc_map_hal_result(hal_rtc_init((hal_rtc_id_t)id, &hal_config));
}

ecos_err_t ecos_rtc_deinit(ecos_rtc_id_t id)
{
    if (id >= ECOS_RTC_ID_COUNT)
        return ECOS_ERR_INVALID_ARGUMENT;

    return rtc_map_hal_result(hal_rtc_deinit((hal_rtc_id_t)id));
}

ecos_err_t ecos_rtc_get_counter(ecos_rtc_id_t id, uint32_t *counter)
{
    if (id >= ECOS_RTC_ID_COUNT || counter == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    return rtc_map_hal_result(hal_rtc_get_counter((hal_rtc_id_t)id, counter));
}

ecos_err_t ecos_rtc_set_counter(ecos_rtc_id_t id, uint32_t counter)
{
    if (id >= ECOS_RTC_ID_COUNT)
        return ECOS_ERR_INVALID_ARGUMENT;

    return rtc_map_hal_result(hal_rtc_set_counter((hal_rtc_id_t)id, counter));
}

ecos_err_t ecos_rtc_set_alarm(ecos_rtc_id_t id, uint32_t alarm)
{
    if (id >= ECOS_RTC_ID_COUNT)
        return ECOS_ERR_INVALID_ARGUMENT;

    return rtc_map_hal_result(hal_rtc_set_alarm((hal_rtc_id_t)id, alarm));
}

int ecos_rtc_alarm_triggered(ecos_rtc_id_t id)
{
    if (id >= ECOS_RTC_ID_COUNT)
        return ECOS_ERR_INVALID_ARGUMENT;

    return rtc_map_hal_result(hal_rtc_alarm_triggered((hal_rtc_id_t)id));
}
