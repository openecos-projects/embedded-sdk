#ifndef ECOS_HAL_RTC_H
#define ECOS_HAL_RTC_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_RTC_ID_0 = 0,
    HAL_RTC_ID_COUNT
} hal_rtc_id_t;

/* 该 RTC 是自由运行计数器加闹钟比较，不是日历钟。
 * prescaler 为输入时钟分频比，硬件写入 PSCR = prescaler - 1；
 * alarm_enabled 非零时同时使能闹钟触发（CTRL bit4）。 */
typedef struct {
    uint32_t prescaler;
    uint8_t alarm_enabled;
} hal_rtc_config_t;

ecos_err_t hal_rtc_init(hal_rtc_id_t id, const hal_rtc_config_t *config);
ecos_err_t hal_rtc_deinit(hal_rtc_id_t id);
ecos_err_t hal_rtc_get_counter(hal_rtc_id_t id, uint32_t *counter);
ecos_err_t hal_rtc_set_counter(hal_rtc_id_t id, uint32_t counter);
ecos_err_t hal_rtc_set_alarm(hal_rtc_id_t id, uint32_t alarm);

/* 触发返回 1，未触发返回 0，失败返回负错误码。 */
int hal_rtc_alarm_triggered(hal_rtc_id_t id);

#ifdef __cplusplus
}
#endif

#endif
