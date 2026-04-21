#include "hal_hp_uart.h"
#include "generated/autoconf.h"
#include "board.h"


void hal_hp_uart_init(uint32_t baudrate){
    REG_UART_1_LCR = 0x00;
    REG_UART_1_DIV = (CONFIG_CPU_FREQ_MHZ * 1000000 / baudrate) - 1;
    REG_UART_1_FCR = 0x0F;
    REG_UART_1_FCR = 0x0C;
    REG_UART_1_LCR = 0x1F;
}

void hal_hp_uart_config(hp_uart_config_t *config){
    REG_UART_1_LCR = 0x00;
    REG_UART_1_DIV = (CONFIG_CPU_FREQ_MHZ * 1000000 / config->baudrate) - 1;
    REG_UART_1_FCR = 0x0F;
    REG_UART_1_FCR = 0x0C;

    REG_UART_1_LCR = (config->data_bits - 5) << 0 |
                     (config->parity << 1) |
                     (config->stop_bits << 2);
}

void hal_hp_uart_send(char c){
    while(((REG_UART_1_LSR & 0x100) >> 8) == 1);
    REG_UART_1_TRX = c;
}


void hal_hp_uart_putstr(char *str){
    while (*str) {
        hal_hp_uart_send(*str++);
    }
}

void hal_hp_uart_recv(char *c){
    while(((REG_UART_1_LSR & 0x080) >> 7) == 1);
    *c = REG_UART_1_TRX;
}

void hal_hp_uart_recv_str(char *str){
    while(1){
        hal_hp_uart_recv(str++);
        if(*(str-1) == '\n'){
            *str = 0;
            break;
        }
    }
}