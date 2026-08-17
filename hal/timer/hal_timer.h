#ifndef __HAL_TIMER_H__
#define __HAL_TIMER_H__

#include <stdint.h>

typedef struct {
    uint32_t period_ticks;
} hal_timer_config_t;

typedef void (*hal_timer_callback_t)(void *arg);

uint8_t hal_delay_us(uint8_t timer_id, uint32_t val);
uint8_t hal_delay_ms(uint8_t timer_id, uint32_t val);
uint8_t hal_delay_s(uint8_t timer_id, uint32_t val);
uint8_t hal_sys_tick_init(uint8_t timer_id);
uint32_t hal_get_sys_tick(uint8_t timer_id);

/**
 * @brief 使用指定周期计数初始化定时器
 */
int hal_timer_init(uint8_t timer_id, const hal_timer_config_t *config);

/**
 * @brief 关闭定时器并清除周期配置
 */
int hal_timer_deinit(uint8_t timer_id);

/**
 * @brief 启动已经初始化的定时器
 */
int hal_timer_start(uint8_t timer_id);

/**
 * @brief 停止定时器并保留周期配置
 */
int hal_timer_stop(uint8_t timer_id);

/**
 * @brief 读取定时器当前计数值
 */
int hal_timer_get_count(uint8_t timer_id, uint32_t *count);

/**
 * @brief 清除定时器锁存中断
 */
int hal_timer_clear_interrupt(uint8_t timer_id);

/**
 * @brief 注册定时器周期中断 callback
 *
 * @note callback 注册期间同一 timer_id 的轮询 delay 和重新初始化会返回失败
 */
int hal_timer_register_callback(uint8_t timer_id,
                                hal_timer_callback_t callback,
                                void *arg,
                                uint32_t priority);

/**
 * @brief 删除定时器周期中断 callback
 */
int hal_timer_unregister_callback(uint8_t timer_id);

#endif
