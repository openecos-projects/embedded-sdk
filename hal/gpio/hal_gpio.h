#ifndef __HAL_GPIO_H__
#define __HAL_GPIO_H__

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
 * @brief 仅适用于C2板卡，GPIO端口写操作更新，需要在设置电平后调用
 * 
 */
void gpio_hal_read_update();

/**
 * @brief  仅适用于C2板卡，GPIO端口读操作更新，需要在获取电平前调用
 * 
 */
void gpio_hal_write_update();

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

/**
 * @brief 配置指定 GPIO 引脚的中断触发类型
 *
 * @param gpio_id GPIO IP 组编号
 * @param gpio_num GPIO 编号
 * @param intr_type 中断触发类型
 * @return int 0 表示成功，-1 表示参数或硬件不支持
 */
int gpio_hal_set_intr_type(uint8_t gpio_id,
                           uint8_t gpio_num,
                           gpio_intr_type_t intr_type);

/**
 * @brief 为指定 GPIO 引脚注册中断处理函数
 *
 * @param gpio_id GPIO IP 组编号
 * @param gpio_num GPIO 编号
 * @param handler 中断处理函数
 * @param arg 传递给中断处理函数的参数
 * @param priority PLIC 中断优先级
 * @return int 0 表示成功，-1 表示失败
 */
int gpio_hal_isr_handler_add(uint8_t gpio_id,
                             uint8_t gpio_num,
                             gpio_hal_isr_t handler,
                             void *arg,
                             uint32_t priority);

/**
 * @brief 删除指定 GPIO 引脚的中断处理函数
 *
 * @param gpio_id GPIO IP 组编号
 * @param gpio_num GPIO 编号
 * @return int 0 表示成功，-1 表示失败
 */
int gpio_hal_isr_handler_remove(uint8_t gpio_id, uint8_t gpio_num);

/**
 * @brief 使能指定 GPIO 引脚的中断
 *
 * @param gpio_id GPIO IP 组编号
 * @param gpio_num GPIO 编号
 * @return int 0 表示成功，-1 表示失败
 */
int gpio_hal_intr_enable(uint8_t gpio_id, uint8_t gpio_num);

/**
 * @brief 禁止指定 GPIO 引脚的中断
 *
 * @param gpio_id GPIO IP 组编号
 * @param gpio_num GPIO 编号
 * @return int 0 表示成功，-1 表示失败
 */
int gpio_hal_intr_disable(uint8_t gpio_id, uint8_t gpio_num);

#endif
