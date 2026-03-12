/**
 * @file board_init.c
 * @brief StarrySky C2 board initialization for hello template
 */

#include "hal/sys_uart.h"
#include "drivers/sys_uart/starrysky_c2/sys_uart_driver.h"

void starrysky_c2_board_init(void)
{
    // Initialize system UART driver for C2
    starrysky_c2_sys_uart_init();
}