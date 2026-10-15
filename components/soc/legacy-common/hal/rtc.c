#include "ecos/hal/rtc.h"
#include "legacy_soc.h"

#include <stddef.h>

ecos_err_t hal_rtc_init(hal_rtc_id_t id, const hal_rtc_config_t *config)
{
    uint32_t control = 2u;
    if (id != HAL_RTC_ID_0 || config == NULL || config->prescaler == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_RTC_0_CTRL = 1u;
    REG_RTC_0_PSCR = config->prescaler - 1u;
    if (config->alarm_enabled != 0u)
        control |= 0x10u;
    REG_RTC_0_CTRL = control;
    return ECOS_OK;
}

ecos_err_t hal_rtc_deinit(hal_rtc_id_t id)
{
    if (id != HAL_RTC_ID_0)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_RTC_0_CTRL = 0u;
    return ECOS_OK;
}

ecos_err_t hal_rtc_get_counter(hal_rtc_id_t id, uint32_t *counter)
{
    if (id != HAL_RTC_ID_0 || counter == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    *counter = REG_RTC_0_CNT;
    return ECOS_OK;
}

ecos_err_t hal_rtc_set_counter(hal_rtc_id_t id, uint32_t counter)
{
    if (id != HAL_RTC_ID_0)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_RTC_0_CNT = counter;
    return ECOS_OK;
}

ecos_err_t hal_rtc_set_alarm(hal_rtc_id_t id, uint32_t alarm)
{
    if (id != HAL_RTC_ID_0)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_RTC_0_ALRM = alarm;
    return ECOS_OK;
}

int hal_rtc_alarm_triggered(hal_rtc_id_t id)
{
    if (id != HAL_RTC_ID_0)
        return ECOS_ERR_INVALID_ARGUMENT;
    return REG_RTC_0_ISTA != 0u ? 1 : 0;
}
