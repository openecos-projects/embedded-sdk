#include "hal/gpio.h"
#include "hal/sys_uart.h"
#include "drivers/gpio/starrysky_c2/gpio_driver.h"
#include "drivers/sys_uart/starrysky_c2/sys_uart_driver.h"

void starrysky_c2_board_init(void)
{
    // Initialize C2 GPIO and sys_uart drivers for GPIO template
    starrysky_c2_gpio_init();
    starrysky_c2_sys_uart_init();
}