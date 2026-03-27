#include "hal_gpio.h"
#include "board.h"

void hal_gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t direction){
    if (direction == GPIO_MODE_INPUT){
        REG_GPIO_0_DDR |= (0x01 << gpio_num);
    }else{
        REG_GPIO_0_DDR &= ~(0x01 << gpio_num);
    }
}


void hal_gpio_set_level(gpio_num_t gpio_num, gpio_level_t level){
    if (level == GPIO_LEVEL_HIGH){
        REG_GPIO_0_DR |= (0x01 << gpio_num);
    }else{
        REG_GPIO_0_DR &= ~(0x01 << gpio_num);
    }
}

int32_t hal_gpio_get_level(gpio_num_t gpio_num){
    return (REG_GPIO_0_DR >> gpio_num) & 0x01;
}
