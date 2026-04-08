#include "hal_hp_uart.h"
#include "hal_hp_uart_type.h"
#include "generated/autoconf.h"
#include "board.h"
#include <stdint.h>

void hal_hp_uart_init(void){
    REG_UART_1_DIV = (uint32_t)(CONFIG_CPU_FREQ_MHZ * 1000000 / CONFIG_UART_BAUD_RATE);
}

void hal_hp_uart_putchar(char c){
    while((REG_UART_1_LSR & 0x20) == 0);
    REG_UART_1_TRX = (uint32_t)c;
}

void hal_hp_uart_putstr(char *str){
    while (*str != '\0'){
        hal_hp_uart_putchar(*str++);
    }
}

char hal_hp_uart_getchar(void){
    while((REG_UART_1_LSR & 0x01) == 0);
    return (char)REG_UART_1_TRX;
}