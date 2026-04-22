#ifndef GPIO_H__
#define GPIO_H__

#include "hal_gpio_type.h"

/**
 * @brief 启用指定GPIO为输入模式
 * 
 * @param gpio_id GPIO IP组编号 (如0表示GPIO_0, 1表示GPIO_1等)
 * @param gpio_num GPIO编号
 */
void gpio_hal_input_enable(uint8_t gpio_id, uint8_t gpio_num);

/**
 * @brief 启用指定GPIO为输出模式
 * 
 * @param gpio_id GPIO IP组编号 (如0表示GPIO_0, 1表示GPIO_1等)
 * @param gpio_num GPIO编号
 */
void gpio_hal_output_enable(uint8_t gpio_id, uint8_t gpio_num);

/**
 * @brief 设置指定GPIO为高电平
 * 
 * @param gpio_id GPIO IP组编号 (如0表示GPIO_0, 1表示GPIO_1等)
 * @param gpio_num GPIO编号
 */
void gpio_hal_set_level(uint8_t gpio_id, uint8_t gpio_num, uint8_t level);

/**
 * @brief 获取指定GPIO的电平
 * 
 * @param gpio_id GPIO IP组编号 (如0表示GPIO_0, 1表示GPIO_1等)
 * @param gpio_num GPIO编号
 * @return uint8_t GPIO电平
 */
uint8_t gpio_hal_get_level(uint8_t gpio_id, uint8_t gpio_num);

/**
 * @brief 更新GPIO端口，需要在设置电平后调用、获取电平前调用
 * 
 */
void gpio_hal_update();

/**
 * @brief 设置指定GPIO的FCFG (Function Configuration)
 * 
 * @param gpio_id GPIO IP组编号 (如0表示GPIO_0, 1表示GPIO_1等)
 * @param gpio_num GPIO编号
 * @param val FCFG值 (通常 0 表示普通GPIO, 1 表示其他功能)
 */
void gpio_hal_set_fcfg(uint8_t gpio_id, uint8_t gpio_num, uint8_t val);

/**
 * @brief 设置指定GPIO的PINMUX
 * 
 * @param gpio_id GPIO IP组编号 (如0表示GPIO_0, 1表示GPIO_1等)
 * @param gpio_num GPIO编号
 * @param val PINMUX值
 */
void gpio_hal_set_mux(uint8_t gpio_id, uint8_t gpio_num, uint8_t val);

#endif