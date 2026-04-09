#ifndef __HAL_HP_UART_TYPE_H__
#define __HAL_HP_UART_TYPE_H__

#include <stdint.h>

typedef enum{
    HP_UART_BAUD_9600    = 9600,
    HP_UART_BAUD_19200   = 19200,
    HP_UART_BAUD_38400   = 38400,
    HP_UART_BAUD_115200  = 115200,
} hp_uart_baud_t;


typedef enum{
    HP_UART_OK     = 0,  
    HP_UART_ERR    = 1,  
} hp_uart_ret_t;

#endif 