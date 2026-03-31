#ifndef __HAL_SYS_UART_H__
#define __HAL_SYS_UART_H__

#include <stdint.h>

/**
 * @brief 初始化系统串口
 * 
 */
void hal_sys_uart_init(void);

/**
 * @brief 发送一个字符到系统串口
 * 
 * @param c 要发送的字符
 */
void hal_sys_putchar(char c);

/**
 * @brief 发送一个字符串到系统串口
 * 
 * @param str 要发送的字符串
 */
void hal_sys_putstr(char *str);

#endif