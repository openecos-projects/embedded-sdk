#include "hal_hp_uart.h"
#include "generated/autoconf.h"
#include "board.h"

void hal_hp_uart_init(uint32_t baudrate){
    REG_GPIO_0_IOFCFG = (uint32_t)0x3;
    REG_GPIO_0_PINMUX = (uint32_t)0;

    REG_UART_1_LCR = 0x00;
    REG_UART_1_DIV = (uint32_t)(100000000 / baudrate - 1);
    REG_UART_1_FCR = (uint32_t)0b1111;
    REG_UART_1_FCR = (uint32_t)0b1100;
    REG_UART_1_LCR = (uint32_t)0b00011111;
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
        if(*(str-1) == '\r' || *(str-1) == '\n'){
            *str = '\0';
            break;
        }
    }
}