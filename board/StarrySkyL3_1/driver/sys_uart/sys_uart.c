#include "hal_sys_uart.h"
#include "generated/autoconf.h"
#include "board.h"

void hal_sys_uart_init(void){
    // Basic 16550 UART init
    // Enable DLAB (set bit 7 of LCR) to access divisor latches
    REG_UART_0_LC = REG_UART_0_LC | 0x80;
    
    // Clear DLAB, set 8 data bits, 1 stop bit, no parity (8N1)
    REG_UART_0_TH = 13;
    REG_UART_0_LC = 0x03;

}

void hal_sys_putchar(char c){
    if (c == '\n') {
        while ((REG_UART_0_LS & 0x20) == 0);
        REG_UART_0_TH = '\r';
    }
    while ((REG_UART_0_LS & 0x20) == 0);
    REG_UART_0_TH = c;

}

void hal_sys_putstr(char *str){
    while (*str != '\0') {
        hal_sys_putchar(*str++);
    }
}
