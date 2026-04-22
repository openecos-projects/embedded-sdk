#ifndef HAL_HP_UART_H__
#define HAL_HP_UART_H__

#include "hal_hp_uart_type.h"
#include <stdint.h>


typedef enum{
    HAL_OK      = 0,
    HAL_ERROR   = 1,
    HAL_BUSY    = 2,
    HAL_TIMEOUT = 3,
} hal_status_t;


void hal_hp_uart_init(uint32_t baudrate);
void hal_hp_uart_config(hp_uart_config_t *config);
void hal_hp_uart_send(char c);
void hal_hp_uart_putstr(char *str);
void hal_hp_uart_recv(char *c);
void hal_hp_uart_recv_str(char *str);

#endif