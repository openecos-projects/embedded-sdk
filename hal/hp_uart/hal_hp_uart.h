#ifndef __HAL_HP_UART_H__
#define __HAL_HP_UART_H__

#include <stdint.h>
#include "hal_hp_uart_type.h"

/**
 * @brief 初始化HP_UART
 * @note 波特率由配置宏自动计算，和SYS_UART初始化逻辑一致
 */
void hal_hp_uart_init(void);

/**
 * @brief 发送一个字符到HP_UART
 * @param c 要发送的字符
 */
void hal_hp_uart_putchar(char c);

/**
 * @brief 发送一个字符串到HP_UART
 * @param str 要发送的字符串
 */
void hal_hp_uart_putstr(char *str);

/**
 * @brief 从HP_UART读取一个字符
 * @return char 读取到的字符
 */
char hal_hp_uart_getchar(void);

#endif 