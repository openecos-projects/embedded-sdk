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

/**
 * @brief 从系统串口接收一个字符
 * 
 * @return uint8_t 接收到的字符
 */
uint8_t hal_sys_getchar(void);

/**
 * 尝试从系统串口读取一个字符。
 *
 * @param data 用于保存读取结果的地址
 * @return 1 表示读取成功，0 表示当前没有数据
 */
uint8_t hal_sys_try_getchar(uint8_t *data);
#endif
