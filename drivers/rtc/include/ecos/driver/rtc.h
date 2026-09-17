#ifndef ECOS_DRIVER_RTC_H
#define ECOS_DRIVER_RTC_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECOS_RTC_ID_0 = 0,
    ECOS_RTC_ID_COUNT
} ecos_rtc_id_t;

#define ECOS_RTC_DEFAULT ((ecos_rtc_id_t)ECOS_RTC_ID_0)

/* 该 RTC 是自由运行计数器加闹钟比较，不是日历钟；
 * prescaler 为输入时钟分频比，1 表示不分频；
 * alarm_enabled 非零时使能闹钟触发。 */
typedef struct {
    uint32_t prescaler;
    uint8_t alarm_enabled;
} ecos_rtc_config_t;

#define ECOS_RTC_CONFIG_DEFAULT \
    { 1u, 0u }

ecos_err_t ecos_rtc_init(ecos_rtc_id_t id, const ecos_rtc_config_t *config);
ecos_err_t ecos_rtc_deinit(ecos_rtc_id_t id);
ecos_err_t ecos_rtc_get_counter(ecos_rtc_id_t id, uint32_t *counter);
ecos_err_t ecos_rtc_set_counter(ecos_rtc_id_t id, uint32_t counter);
ecos_err_t ecos_rtc_set_alarm(ecos_rtc_id_t id, uint32_t alarm);

/* 闹钟触发返回 1，未触发返回 0，失败返回负错误码。 */
int ecos_rtc_alarm_triggered(ecos_rtc_id_t id);

#ifdef __cplusplus
}
#endif

#endif
