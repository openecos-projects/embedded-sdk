#include "hal_gpio.h"
#include "hal_gpio_type.h"
#include "board.h"
#include "stdio.h"
#include <stdint.h>
#include "generated/autoconf.h"

void gpio_hal_input_enable(uint8_t gpio_id, uint8_t gpio_num){
    uint8_t io_group = gpio_id;
    uint8_t io_offset = gpio_num % 32;
    switch (io_group){
        case 0:
            REG_GPIO_0_PADDIR = REG_GPIO_0_PADDIR & ~(0x01 << io_offset);
            break;
#ifdef GPIO_GROUP_1
        case 1:
            REG_GPIO_1_PADDIR = REG_GPIO_1_PADDIR & ~(0x01 << io_offset);
            break;
#endif
        default:
            break;
    }
}

void gpio_hal_output_enable(uint8_t gpio_id, uint8_t gpio_num){
    uint8_t io_group = gpio_id;
    uint8_t io_offset = gpio_num % 32;
    switch (io_group){
        case 0:
            REG_GPIO_0_PADDIR = REG_GPIO_0_PADDIR | (0x01 << io_offset);
            break;
#ifdef GPIO_GROUP_1
        case 1:
            REG_GPIO_1_PADDIR = REG_GPIO_1_PADDIR | (0x01 << io_offset);
            break;
#endif
        default:
            break;
    }
}

void gpio_hal_set_level(uint8_t gpio_id, uint8_t gpio_num, uint8_t level){
    uint8_t io_group = gpio_id;
    uint8_t io_offset = gpio_num % 32;
    switch (io_group){
        case 0:
            if (level == GPIO_LEVEL_HIGH){
                REG_GPIO_0_PADOUT |= ((uint32_t)0x01 << io_offset);
            }
            else{
                REG_GPIO_0_PADOUT &= ~((uint32_t)0x01 << io_offset);
            }
            break;
        case 1:
            if (level == GPIO_LEVEL_HIGH){
                REG_GPIO_1_PADOUT |= ((uint32_t)0x01 << io_offset);
            }
            else{
                REG_GPIO_1_PADOUT &= ~((uint32_t)0x01 << io_offset);
            }
            break;
        default:
            break;
    }
}

uint8_t gpio_hal_get_level(uint8_t gpio_id, uint8_t gpio_num){
    uint8_t io_group = gpio_id;
    uint8_t io_offset = gpio_num % 32;
    switch (io_group){
        case 0:
            return (REG_GPIO_0_PADIN & (1 << io_offset)) ? 1 : 0;
        case 1:
            return (REG_GPIO_1_PADIN & (1 << io_offset)) ? 1 : 0;
        default:
            return 0;
    }
}

void gpio_hal_set_fcfg(uint8_t gpio_id, uint8_t gpio_num, uint8_t val){
    uint8_t io_group = gpio_id;
    uint8_t io_offset = gpio_num % 32;
    switch (io_group){
        case 0:
            if(val == 1){
                REG_GPIO_0_IOFCFG = REG_GPIO_0_IOFCFG | (1 << io_offset);
            }else{
                REG_GPIO_0_IOFCFG = REG_GPIO_0_IOFCFG & ~(1 << io_offset);
            }
            break;
        case 1:
            if(val == 1){
                REG_GPIO_1_IOFCFG = REG_GPIO_1_IOFCFG | (1 << io_offset);
            }else{
                REG_GPIO_1_IOFCFG = REG_GPIO_1_IOFCFG & ~(1 << io_offset);
            }
            break;
        default:
            break;
    }
}

void gpio_hal_set_mux(uint8_t gpio_id, uint8_t gpio_num, uint8_t val){
    uint8_t io_group = gpio_id;
    uint8_t io_offset = gpio_num % 32;
    switch (io_group){
        case 0:
            if(val == 1){
                REG_GPIO_0_PINMUX = REG_GPIO_0_PINMUX | (1 << io_offset);
            }else{
                REG_GPIO_0_PINMUX = REG_GPIO_0_PINMUX & ~(1 << io_offset);
            }
            break;
#ifdef GPIO_GROUP_1
        case 1:
            if(val == 1){
                REG_GPIO_1_PINMUX = REG_GPIO_1_PINMUX | (1 << io_offset);
            }else{
                REG_GPIO_1_PINMUX = REG_GPIO_1_PINMUX & ~(1 << io_offset);
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
