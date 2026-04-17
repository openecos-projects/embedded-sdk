#include "main.h"
#include "stddef.h"

void main(void){
    
    hal_sys_uart_init();
    hal_sys_putstr("GPIO TEST!\n\r");

    gpio_hal_output_enable(0,GPIO_NUM_0);

    while(1){
        gpio_hal_set_level(0,GPIO_NUM_0,GPIO_LEVEL_HIGH);
        hal_delay_s(1,1);
        gpio_hal_set_level(0,GPIO_NUM_0,GPIO_LEVEL_LOW);
        hal_delay_s(1,1);
    }

}