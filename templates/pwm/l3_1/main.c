#include "main.h"
#include "hal_sys_uart.h"
#include "hal_pwm.h"
#include "hal_gpio.h"
#include "hal_timer.h"

void main(void){
    
    hal_sys_uart_init();
    printf("PWM test\n");

    // Configure GPIO 2 for PWM
    gpio_hal_set_fcfg(0, 2, 1);
    gpio_hal_set_mux(0, 2, 0);

    pwm_config_t pwm_config = {
        .pscr = 72 - 1,
        .cmp = 1000 - 1,
    };
    pwm_hal_init(NULL, 0, &pwm_config);
    pwm_hal_enable(NULL, 0);

    while(1){
        for(uint32_t i = 0; i < 990; i+=5){
            pwm_hal_set_compare(NULL, 0, PWM_CH0, i);
            hal_delay_ms(0, 5);
        }
        for(uint32_t i = 990; i > 0; i-=5){
            pwm_hal_set_compare(NULL, 0, PWM_CH0, i);
            hal_delay_ms(0, 5);
        }
    }
}
