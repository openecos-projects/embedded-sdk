#include "ecos/hal/rtc.h"
#include "ysyx_2512_1_soc.h"

#include <stddef.h>

#define RTC_CTRL_ENABLE         0x1u
#define RTC_CTRL_COUNT_ENABLE   0x2u
#define RTC_CTRL_ALARM_ENABLE   0x10u

static int rtc_id_is_valid(hal_rtc_id_t id)
{
    return id == HAL_RTC_ID_0;
}

ecos_err_t hal_rtc_init(hal_rtc_id_t id, const hal_rtc_config_t *config)
{
    uint32_t ctrl;

    if (!rtc_id_is_valid(id) || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (config->prescaler == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    /* 沿用 2.x 实测序列：先置 bit0 进入配置，写分频后再开计数。 */
    REG_RTC_0_CTRL = RTC_CTRL_ENABLE;
    REG_RTC_0_PSCR = config->prescaler - 1u;

    ctrl = RTC_CTRL_COUNT_ENABLE;
    if (config->alarm_enabled != 0u)
        ctrl |= RTC_CTRL_ALARM_ENABLE;
    REG_RTC_0_CTRL = ctrl;
    return ECOS_OK;
}

ecos_err_t hal_rtc_deinit(hal_rtc_id_t id)
{
    if (!rtc_id_is_valid(id))
        return ECOS_ERR_INVALID_ARGUMENT;

    REG_RTC_0_CTRL = 0u;
    return ECOS_OK;
}

ecos_err_t hal_rtc_get_counter(hal_rtc_id_t id, uint32_t *counter)
{
    if (!rtc_id_is_valid(id) || counter == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    *counter = REG_RTC_0_CNT;
    return ECOS_OK;
}

ecos_err_t hal_rtc_set_counter(hal_rtc_id_t id, uint32_t counter)
{
    if (!rtc_id_is_valid(id))
        return ECOS_ERR_INVALID_ARGUMENT;

    REG_RTC_0_CNT = counter;
    return ECOS_OK;
}

ecos_err_t hal_rtc_set_alarm(hal_rtc_id_t id, uint32_t alarm)
{
    if (!rtc_id_is_valid(id))
        return ECOS_ERR_INVALID_ARGUMENT;

    REG_RTC_0_ALRM = alarm;
    return ECOS_OK;
}

int hal_rtc_alarm_triggered(hal_rtc_id_t id)
{
    if (!rtc_id_is_valid(id))
        return ECOS_ERR_INVALID_ARGUMENT;

    return (REG_RTC_0_ISTA != 0u) ? 1 : 0;
}
