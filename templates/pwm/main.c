#include "main.h"
#include "board_init.h"

// Delay function implementation
void delay_ms(uint32_t ms) {
    // Simple delay loop (adjust based on actual clock frequency)
    for (volatile uint32_t i = 0; i < ms * 1000; i++) {
        __asm__ volatile ("nop");
    }
}

void main(void){
    // Initialize board-specific drivers
    starrysky_c2_board_init();
    
    // Get HAL driver instances
    sys_uart_driver_t* sys_uart_drv = sys_uart_get_driver();
    pwm_driver_t* pwm_drv = pwm_get_driver();
    
    // Output test message
    sys_uart_drv->puts("PWM test\n");

    // Configure PWM
    pwm_config_t pwm_config = {
        .pscr = 72 - 1,
        .cmp = 1000 - 1,
    };
    pwm_drv->init(0, &pwm_config);

    // PWM fade effect
    while(1){
        for(uint32_t i = 0; i < 990; i+=5){
            pwm_drv->set_compare(PWM_CH0, i);
            delay_ms(5);
        }
        for(uint32_t i = 990; i > 0; i-=5){
            pwm_drv->set_compare(PWM_CH0, i);
            delay_ms(5);
        }
    }
}