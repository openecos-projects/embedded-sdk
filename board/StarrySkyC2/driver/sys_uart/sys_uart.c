#include "hal_sys_uart.h"
#include "generated/autoconf.h"
#include "board.h"

void hal_sys_uart_init(void){
    REG_UART_0_CLKDIV =  (uint32_t)(CONFIG_CPU_FREQ_MHZ * 1000000 / CONFIG_UART_BAUD_RATE);
}

void hal_sys_putchar(char c){
    REG_UART_0_DATA = c;
}

void hal_sys_putstr(char *str){
    while (*str!='\0') {
        hal_sys_putchar(*str++);
    }
}