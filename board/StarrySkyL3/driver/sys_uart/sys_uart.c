#include "hal_sys_uart.h"
#include "board.h"

void hal_sys_uart_init(void) {
    REG_UART_0_LC |= 0x80;
    REG_UART_0_TH = 13;
    REG_UART_0_IE = 0;
    REG_UART_0_LC = 0x03;
}

void hal_sys_putchar(char c) {
    if (c == '\n') {
        while ((REG_UART_0_LS & 0x20) == 0) {
        }
        REG_UART_0_TH = '\r';
    }
    while ((REG_UART_0_LS & 0x20) == 0) {
    }
    REG_UART_0_TH = (uint8_t)c;
}

void hal_sys_putstr(char *str) {
    while (*str != '\0') {
        hal_sys_putchar(*str++);
    }
}

uint8_t hal_sys_getchar(void) {
    while ((REG_UART_0_LS & 0x01) == 0) {
    }
    return REG_UART_0_RB;
}
