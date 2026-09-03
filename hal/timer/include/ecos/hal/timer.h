#ifndef ECOS_HAL_TIMER_H
#define ECOS_HAL_TIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t hal_timer_id_t;

typedef struct {
    uint32_t period_ticks;
} hal_timer_config_t;

enum {
    HAL_TIMER_OK = 0,
    HAL_TIMER_ERROR_INVALID_ARGUMENT = -1,
    HAL_TIMER_ERROR_UNSUPPORTED = -2,
    HAL_TIMER_ERROR_NOT_INITIALIZED = -3
};

/* Timer 周期和计数值均使用一微秒计数单位。 */
int hal_timer_get_instance_count(void);
int hal_timer_init(hal_timer_id_t timer, const hal_timer_config_t *config);
int hal_timer_deinit(hal_timer_id_t timer);
int hal_timer_start(hal_timer_id_t timer);
int hal_timer_stop(hal_timer_id_t timer);
int hal_timer_get_count(hal_timer_id_t timer, uint32_t *count);

/* 到期返回 1，未到期返回 0，失败返回负错误码。 */
int hal_timer_is_expired(hal_timer_id_t timer);

/* 单次轮询延时，返回前停止并释放所选 Timer。 */
int hal_timer_delay_us(hal_timer_id_t timer, uint32_t duration_us);

#ifdef __cplusplus
}
#endif

#endif
