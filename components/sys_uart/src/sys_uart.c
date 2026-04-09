#include "sys_uart.h"
#include "generated/autoconf.h"
#include "board.h"

void sys_uart_init(void){
#if CONFIG_SYS_UART_IP_ID == 0
    REG_UART_0_CLKDIV =  (uint32_t)(CONFIG_CPU_FREQ_MHZ * 1000000 / CONFIG_UART_BAUD_RATE);
#elif CONFIG_SYS_UART_IP_ID == 1
    REG_UART_0_LC = REG_UART_0_LC | 0x80;
    REG_UART_0_TH = 13 ; // 25M 115200bps
    REG_UART_0_LC = 0x03; // OpenCores UART16550 core spec ver0.6 4.9 section
#endif
}

void sys_putchar(char c){
#if CONFIG_SYS_UART_IP_ID == 0
    REG_UART_0_DATA = c;
#elif CONFIG_SYS_UART_IP_ID == 1
    uint8_t val;
    do
    {
        val = REG_UART_0_LS;
    } while ((val & 0x20) == 0);
    REG_UART_0_TH = c;
#endif
}

void sys_putstr(char *str){
    while (*str!='\0') {
        sys_putchar(*str++);
    }
}