#include "hal_gpio.h"
#include "hal_gpio_type.h"
#include "board.h"
#include <stdint.h>
#include "generated/autoconf.h"


void gpio_hal_input_enable(void *hal, uint8_t gpio_num){
    
    REG_GPIO_0_DDR |= (0x01 << gpio_num);
}


void gpio_hal_output_enable(void *hal, uint8_t gpio_num){

    REG_GPIO_0_DDR &= ~(0x01 << gpio_num);
}


void gpio_hal_set_level(void *hal, uint8_t gpio_num, uint8_t level){
    
    if (level == GPIO_LEVEL_HIGH)
    {
        REG_GPIO_0_DR |= (1 << gpio_num);
    }
    else
    {
        REG_GPIO_0_DR &= ~(1 << gpio_num);
    }
}


uint8_t gpio_hal_get_level(void *hal, uint8_t gpio_num){  
    return (REG_GPIO_0_DR & (1 << gpio_num)) ? 1 :0;
}