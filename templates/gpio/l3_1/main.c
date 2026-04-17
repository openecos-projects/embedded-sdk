#include "main.h"
#include "hal_sys_uart.h"
#include "hal_gpio.h"

void delay(volatile uint32_t count) {
    while(count--) {
        // empty loop
    }
}

int main()
{
    hal_sys_putstr("StarrySkyL3_1 GPIO Blink Test!\n\r");

    // Enable output for GPIO_0_26 and GPIO_0_27
    gpio_hal_output_enable(0, GPIO_NUM_26);
    gpio_hal_output_enable(0, GPIO_NUM_27);

    while(1){
        // Turn on LED 1 (GPIO_0_26 High), Turn off LED 2 (GPIO_0_27 Low)
        gpio_hal_set_level(0, GPIO_NUM_26, GPIO_LEVEL_HIGH);
        gpio_hal_set_level(0, GPIO_NUM_27, GPIO_LEVEL_LOW);
        delay(1000000);
        
        // Turn off LED 1 (GPIO_0_26 Low), Turn on LED 2 (GPIO_0_27 High)
        gpio_hal_set_level(0, GPIO_NUM_26, GPIO_LEVEL_LOW);
        gpio_hal_set_level(0, GPIO_NUM_27, GPIO_LEVEL_HIGH);
        delay(1000000);
    }

    return 0;
}