#include "hal_hp_uart.h"
#include "generated/autoconf.h"
#include "board.h"

uint8_t hp_uart_hal_config(uint8_t uart_id, hp_uart_config_t* config){
    if(uart_id != HAL_UART_PORT_0) {
        return 1;
    }
    // 禁用UART
    REG_UART_1_LCR = 0x00;

    // 设置波特率
    REG_UART_1_DIV = (CONFIG_CPU_FREQ_MHZ * 1000000 / config->baudrate) - 1;

    // 启用FIFO和接收中断
    REG_UART_1_FCR = 0x0F;  
    REG_UART_1_FCR = 0x0C;

    // 设置数据位，校验位，停止位
    REG_UART_1_LCR = (config->data_bits - 5) << 0 |
                     (config->parity << 1) |
                     (config->stop_bits << 2);
        
    return 0;
}

uint8_t hp_uart_hal_send(uint8_t uart_id, char c){
    while (((REG_UART_1_LSR & 0x100) >> 8) == 1);
    REG_UART_1_TRX = c;
    return 0;
}

uint8_t hp_uart_hal_send_str(uint8_t uart_id, char* str){
    while (*str){
        hp_uart_hal_send(0, *str++);
    }
    return 0;
}

uint8_t hp_uart_hal_recv(uint8_t uart_id, char* c){
    while (((REG_UART_1_LSR & 0x100) >> 8) == 0);
    *c = REG_UART_1_RRX;
    return 0;
}

uint8_t hp_uart_hal_recv_str(uint8_t uart_id, char* str){
    while (1){
        hp_uart_hal_recv(uart_id, str++);
        if (*str == '\n'){
            break;
        }
    }
    return 0;
}
