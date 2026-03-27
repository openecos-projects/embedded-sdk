#ifndef GPIO_H__
#define GPIO_H__

#include "hal_gpio_type.h"


/**
 * @brief 设置单个GPIO引脚方向函数
 * 
 * 设置指定GPIO引脚的方向为输入或输出。
 * 
 * @param gpio_num GPIO引脚号，枚举类型gpio_num_t
 * @param direction GPIO方向，枚举类型gpio_mode_t
 */
void hal_gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t direction);

/**
 * @brief GPIO设置电平函数
 * 
 * 设置指定GPIO引脚的电平为高或低。
 * 
 * @param gpio_num GPIO引脚号，枚举类型gpio_num_t
 * @param level GPIO电平，枚举类型gpio_level_t
 */
void hal_gpio_set_level(gpio_num_t gpio_num, gpio_level_t level);

/**
 * @brief GPIO获取电平函数
 * 
 * 获取指定GPIO引脚的当前电平。
 * 
 * @param gpio_num GPIO引脚号，枚举类型gpio_num_t
 * @return int32_t GPIO电平，0表示低电平，1表示高电平
 */
int32_t hal_gpio_get_level(gpio_num_t gpio_num);

#endif