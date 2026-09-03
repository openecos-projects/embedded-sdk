#ifndef ECOS_DRIVER_TIMER_H
#define ECOS_DRIVER_TIMER_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ecos_timer_id_t;

#define ECOS_TIMER_DEFAULT ((ecos_timer_id_t)0u)

typedef struct {
    uint32_t period_us;
} ecos_timer_config_t;

#define ECOS_TIMER_CONFIG_DEFAULT \
    { 1000u }

/* 返回当前 Target 提供的 Timer 实例数量。 */
int ecos_timer_get_instance_count(void);

ecos_err_t ecos_timer_init(ecos_timer_id_t timer,
                           const ecos_timer_config_t *config);
ecos_err_t ecos_timer_deinit(ecos_timer_id_t timer);

/* start 会从零重新开始已经配置的周期。 */
ecos_err_t ecos_timer_start(ecos_timer_id_t timer);
ecos_err_t ecos_timer_stop(ecos_timer_id_t timer);

/* 读取原始计数值；未接出计数值的 Target 返回 UNSUPPORTED。 */
ecos_err_t ecos_timer_get_count(ecos_timer_id_t timer, uint32_t *count);

/* 到期返回 1，未到期返回 0，失败返回负错误码。 */
int ecos_timer_is_expired(ecos_timer_id_t timer);

/* 轮询延时会临时替换所选 Timer 实例的配置。 */
ecos_err_t ecos_timer_delay_us(ecos_timer_id_t timer, uint32_t duration_us);
ecos_err_t ecos_timer_delay_ms(ecos_timer_id_t timer, uint32_t duration_ms);
ecos_err_t ecos_timer_delay_s(ecos_timer_id_t timer, uint32_t duration_s);

#ifdef __cplusplus
}
#endif

#endif
