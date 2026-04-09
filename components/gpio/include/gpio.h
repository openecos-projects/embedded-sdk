#ifndef GPIO_H__
#define GPIO_H__

#include "gpio_type.h"

/**
 * @brief GPIO配置函数
 * 
 * 根据给定的配置结构体，设置GPIO的引脚模式和方向。
 * 
 * @param config 指向gpio_config_t结构体的指针，包含引脚位掩码和模式信息
 */
void gpio_config(const gpio_config_t* config);

/**
 * @brief 设置单个GPIO引脚方向函数
 * 
 * 设置指定GPIO引脚的方向为输入或输出。
 * 
 * @param gpio_num GPIO引脚号，枚举类型gpio_num_t
 * @param direction GPIO方向，枚举类型gpio_mode_t
 */
void gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t direction);

/**
 * @brief GPIO设置电平函数
 * 
 * 设置指定GPIO引脚的电平为高或低。
 * 
 * @param gpio_num GPIO引脚号，枚举类型gpio_num_t
 * @param level GPIO电平，枚举类型gpio_level_t
 */
void gpio_set_level(gpio_num_t gpio_num, gpio_level_t level);

/**
 * @brief GPIO获取电平函数
 * 
 * 获取指定GPIO引脚的当前电平。
 * 
 * @param gpio_num GPIO引脚号，枚举类型gpio_num_t
 * @return int32_t GPIO电平，0表示低电平，1表示高电平
 */
int32_t gpio_get_level(gpio_num_t gpio_num);

/**
 * @brief GPIO配置单个引脚复用功能
 * 
 * 根据给定的引脚号和复用功能，配置GPIO引脚的复用功能。
 * 
 * @param gpio_num GPIO引脚号，枚举类型gpio_num_t
 * @param func GPIO复用功能，枚举类型gpio_function_t
 */
void gpio_set_function(gpio_num_t gpio_num, gpio_func_t func);
#endif