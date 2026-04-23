#include "hal_gpio.h"
#include "hal_gpio_type.h"
#include "board.h"
#include <stdint.h>
#include "generated/autoconf.h"
#include "hal_sys_uart.h"
#include "assert.h"


static volatile uint32_t ddr,dr,rdr; // 不可重入的变量
void gpio_hal_input_enable(uint8_t gpio_id, uint8_t gpio_num){
    
    ddr |= (1 << gpio_num);
}


void gpio_hal_output_enable(uint8_t gpio_id, uint8_t gpio_num){

    ddr &= ~(1 << gpio_num);
}


void gpio_hal_set_level(uint8_t gpio_id, uint8_t gpio_num, uint8_t level){
    if (level == GPIO_LEVEL_HIGH)
    {
        dr |= (1 << gpio_num);
    }
    else
    {
        dr &= ~(1 << gpio_num);
    }
}

uint8_t gpio_hal_get_level(uint8_t gpio_id, uint8_t gpio_num){  
    return (dr & (1 << gpio_num)) ? 1 : 0;
}

void gpio_hal_read_update(){
    rdr = REG_GPIO_0_DR;
    dr = dr | rdr; // 读取时，之前写的部分会左移到高16bit，现在读的部分在低16bit
    gpio_hal_write_update();
    hal_delay_ms(0,5);
}

void gpio_hal_write_update(){
    REG_GPIO_0_DDR = ddr;
    REG_GPIO_0_DR = dr;
    hal_delay_ms(0,5);
}

void gpio_hal_set_fcfg(uint8_t gpio_id, uint8_t gpio_num, uint8_t val){
    // StarrySkyC2 does not implement FCFG
    assert(0,"[Warn] StarrySkyC2 does not implement FCFG\n");
}

void gpio_hal_set_mux(uint8_t gpio_id, uint8_t gpio_num, uint8_t val){
    // StarrySkyC2 does not implement PINMUX
    assert(0,"[Warn] StarrySkyC2 does not implement FCFG\n");
}