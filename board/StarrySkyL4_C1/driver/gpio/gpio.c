#include "hal_gpio.h"
#include "hal_gpio_type.h"
#include "board.h"
#include "stdio.h"
#include <stdint.h>
#include "generated/autoconf.h"

static uint32_t gpio_bit(uint8_t gpio_num)
{
    return (uint32_t)1u << (gpio_num % 32u);
}

void gpio_hal_input_enable(uint8_t gpio_id, uint8_t gpio_num){
    uint8_t io_group = gpio_id;
    uint32_t bit = gpio_bit(gpio_num);
    switch (io_group){
        case 0:
            REG_GPIO_0_PADDIR &= ~bit;
            break;
#ifdef GPIO_GROUP_1
        case 1:
            REG_GPIO_1_PADDIR &= ~bit;
            break;
#endif
#ifdef GPIO_GROUP_2
        case 2:
            REG_GPIO_2_PADDIR &= ~bit;
            break;
#endif
        default:
            break;
    }
}

void gpio_hal_output_enable(uint8_t gpio_id, uint8_t gpio_num){
    uint8_t io_group = gpio_id;
    uint32_t bit = gpio_bit(gpio_num);
    switch (io_group){
        case 0:
            REG_GPIO_0_PADDIR |= bit;
            break;
#ifdef GPIO_GROUP_1
        case 1:
            REG_GPIO_1_PADDIR |= bit;
            break;
#endif
#ifdef GPIO_GROUP_2
        case 2:
            REG_GPIO_2_PADDIR |= bit;
            break;
#endif
        default:
            break;
    }
}

void gpio_hal_set_level(uint8_t gpio_id, uint8_t gpio_num, uint8_t level){
    uint8_t io_group = gpio_id;
    uint32_t bit = gpio_bit(gpio_num);
    switch (io_group){
        case 0:
            if (level == GPIO_LEVEL_HIGH){
                REG_GPIO_0_PADOUT |= bit;
            }
            else{
                REG_GPIO_0_PADOUT &= ~bit;
            }
            break;
#ifdef GPIO_GROUP_1
        case 1:
            if (level == GPIO_LEVEL_HIGH){
                REG_GPIO_1_PADOUT |= bit;
            }
            else{
                REG_GPIO_1_PADOUT &= ~bit;
            }
            break;
#endif
#ifdef GPIO_GROUP_2
        case 2:
            if (level == GPIO_LEVEL_HIGH){
                REG_GPIO_2_PADOUT |= bit;
            }
            else{
                REG_GPIO_2_PADOUT &= ~bit;
            }
            break;
#endif
        default:
            break;
    }
}

uint8_t gpio_hal_get_level(uint8_t gpio_id, uint8_t gpio_num){
    uint8_t io_group = gpio_id;
    uint32_t bit = gpio_bit(gpio_num);
    switch (io_group){
        case 0:
            return (REG_GPIO_0_PADIN & bit) ? 1u : 0u;
#ifdef GPIO_GROUP_1
        case 1:
            return (REG_GPIO_1_PADIN & bit) ? 1u : 0u;
#endif
#ifdef GPIO_GROUP_2
        case 2:
            return (REG_GPIO_2_PADIN & bit) ? 1u : 0u;
#endif
        default:
            return 0;
    }
}

void gpio_hal_set_fcfg(uint8_t gpio_id, uint8_t gpio_num, uint8_t val){
    uint8_t io_group = gpio_id;
    uint32_t bit = gpio_bit(gpio_num);
    switch (io_group){
        case 0:
            if(val == 1){
                REG_GPIO_0_IOFCFG |= bit;
            }else{
                REG_GPIO_0_IOFCFG &= ~bit;
            }
            break;
#ifdef GPIO_GROUP_1
        case 1:
            if(val == 1){
                REG_GPIO_1_IOFCFG |= bit;
            }else{
                REG_GPIO_1_IOFCFG &= ~bit;
            }
            break;
#endif
#ifdef GPIO_GROUP_2
        case 2:
            if(val == 1){
                REG_GPIO_2_IOFCFG |= bit;
            }else{
                REG_GPIO_2_IOFCFG &= ~bit;
            }
            break;
#endif
        default:
            break;
    }
}

void gpio_hal_set_mux(uint8_t gpio_id, uint8_t gpio_num, uint8_t val){
    uint8_t io_group = gpio_id;
    uint32_t bit = gpio_bit(gpio_num);
    switch (io_group){
        case 0:
            if(val == 1){
                REG_GPIO_0_PINMUX |= bit;
            }else{
                REG_GPIO_0_PINMUX &= ~bit;
            }
            break;
#ifdef GPIO_GROUP_1
        case 1:
            if(val == 1){
                REG_GPIO_1_PINMUX |= bit;
            }else{
                REG_GPIO_1_PINMUX &= ~bit;
            }
            break;
#endif
#ifdef GPIO_GROUP_2
        case 2:
            if(val == 1){
                REG_GPIO_2_PINMUX |= bit;
            }else{
                REG_GPIO_2_PINMUX &= ~bit;
            }
            break;
#endif
        default:
            break;
    }
}

void gpio_hal_read_update(){

}

void gpio_hal_write_update(){

}
