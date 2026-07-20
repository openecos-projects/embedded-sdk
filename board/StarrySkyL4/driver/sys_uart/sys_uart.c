#include "hal_sys_uart.h"
#include "generated/autoconf.h"
#include "board.h"

void hal_sys_uart_init(void){
    REG_UART_0_LC = REG_UART_0_LC | 0x80;
    REG_UART_0_TH = 13;
    REG_UART_0_IE = 0;
    REG_UART_0_LC = 0x03;
    REG_UART_0_IE = 0;
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

uint8_t hal_sys_getchar(void){
    // Wait until Data Ready (DR) bit (bit 0) of Line Status Register (LSR) is set
    while ((REG_UART_0_LS & 0x01) == 0);
    // Read and return the character from Receiver Buffer Register (RBR)
    return REG_UART_0_RB;
}
