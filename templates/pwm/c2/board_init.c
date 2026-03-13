#include "hal/sys_uart.h"
#include "hal/pwm.h"
#include "drivers/sys_uart/starrysky_c2/sys_uart_driver.h"
#include "drivers/pwm/starrysky_c2/pwm_driver.h"

void starrysky_c2_board_init(void)
{
    // Initialize C2 system UART and PWM drivers
    starrysky_c2_sys_uart_init();
    starrysky_c2_pwm_init();
}