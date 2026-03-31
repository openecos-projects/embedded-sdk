#ifndef GPIO_H__
#define GPIO_H__

#include "hal_gpio_type.h"

/**
 * @brief 启用指定GPIO为输入模式
 * 
 * @param hal GPIO HAL实例指针[暂时未使用]
 * @param gpio_num GPIO编号
 */
void gpio_hal_input_enable(void *hal, uint8_t gpio_num);

/**
 * @brief 启用指定GPIO为输出模式
 * 
 * @param hal GPIO HAL实例指针[暂时未使用]
 * @param gpio_num GPIO编号
 */
void gpio_hal_output_enable(void *hal, uint8_t gpio_num);

/**
 * @brief 设置指定GPIO为高电平
 * 
 * @param hal GPIO HAL实例指针[暂时未使用]
 * @param gpio_num GPIO编号
 */
void gpio_hal_set_level(void *hal, uint8_t gpio_num, uint8_t level);

/**
 * @brief 获取指定GPIO的电平
 * 
 * @param hal GPIO HAL实例指针[暂时未使用]
 * @param gpio_num GPIO编号
 * @return uint8_t GPIO电平
 */
uint8_t gpio_hal_get_level(void *hal, uint8_t gpio_num);

#endif