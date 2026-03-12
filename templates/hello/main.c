#include "main.h"
#include "hal/sys_uart.h"

void main(void)
{
    // Initialize board-specific drivers
    starrysky_c2_board_init();
    
    // Get HAL driver instance
    sys_uart_driver_t* sys_uart_drv = sys_uart_get_driver();
    
    // Use HAL driver to output message
    sys_uart_drv->puts("Hello, World!\n");
}