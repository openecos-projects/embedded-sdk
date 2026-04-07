#ifndef __HAL_PWM_H__
#define __HAL_PWM_H__

#include "hal_pwm_type.h"
#include <stdint.h>

/**
 * @brief 指定timer_id的PWM模块
 * 
 * @param hal PWM HAL实例指针
 * @param timer_id PWM对应的定时器ID
 * @param config PWM配置结构体指针
 * @return uint8_t 0:成功 1:失败
 */
uint8_t pwm_hal_init(void *hal, uint8_t timer_id, pwm_config_t *config);

/**
 * @brief 设置指定timer_id、指定通道的PWM比较值
 * 
 * @param hal PWM HAL实例指针
 * @param timer_id PWM对应的定时器ID
 * @param ch PWM通道(PWM_CH0~PWM_CH3)
 * @param cmp 比较值
 * @return uint8_t 0:成功 1:失败
 */
uint8_t pwm_hal_set_compare(void *hal, uint8_t timer_id, pwm_channel_t ch, uint32_t cmp);

/**
 * @brief 使能指定timer_id的PWM模块
 * 
 * @param hal PWM HAL实例指针
 * @param timer_id PWM对应的定时器ID
 * @return uint8_t 0:成功 1:失败
 */
uint8_t pwm_hal_enable(void *hal, uint8_t timer_id);

/**
 * @brief 失能指定timer_id的PWM模块
 * 
 * @param hal PWM HAL实例指针
 * @param timer_id PWM对应的定时器ID
 * @return uint8_t 0:成功 1:失败
 */
uint8_t pwm_hal_disable(void *hal, uint8_t timer_id);

#endif 