#ifndef __HAL_HP_UART_H__
#define __HAL_HP_UART_H__

#include <stdint.h>
#include "hal_hp_uart_type.h"

/**
 * @brief 配置串口
 * 
 * @param uart_id 串口ID
 * @param config 串口配置
 * @return uint8_t 0 成功，1 失败
 */
uint8_t hp_uart_hal_config(uint8_t uart_id, hp_uart_config_t* config);

/**
 * @brief 发送一个字符到串口
 * 
 * @param uart_id 串口ID
 * @param c 要发送的字符
 * @return uint8_t 0 成功，1 失败
 */
uint8_t hp_uart_hal_send(uint8_t uart_id, char c);

/**
 * @brief 发送一个字符串到串口
 * 
 * @param uart_id 串口ID
 * @param str 要发送的字符串
 * @return uint8_t 0 成功，1 失败
 */
uint8_t hp_uart_hal_send_str(uint8_t uart_id, char* str);

/**
 * @brief 接收一个字符从串口
 * 
 * @param uart_id 串口ID
 * @param c 接收的字符
 * @return uint8_t 0 成功，1 失败
 */
uint8_t hp_uart_hal_recv(uint8_t uart_id, char* c);

/**
 * @brief 接收一个字符串从串口
 * 
 * @param uart_id 串口ID
 * @param str 接收的字符串
 * @return uint8_t 0 成功，1 失败
 */
uint8_t hp_uart_hal_recv_str(uint8_t uart_id, char* str);
#endif