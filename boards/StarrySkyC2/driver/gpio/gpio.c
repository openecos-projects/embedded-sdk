#include "hal_gpio.h"
#include "hal_gpio_type.h"
#include "board.h"
#include <stdint.h>
#include "generated/autoconf.h"
#include "hal_sys_uart.h"
#include "assert.h"
#include "hal_timer.h"

// C2板卡在读GPIO寄存器时会产生硬件错误，具体行为分析如下：
// 读取时，之前写的部分会左移到高16bit，现在读的部分在低16bit
// 由于用软件来消除每一个bit的数据冒险会增加复杂度、增加寄存器翻转延迟
// 因此使用ddr和wdr来维护ddr和dr的读-改-写操作，维护rdr来实现dr的读操作

static volatile uint32_t ddr,wdr,rdr; // 不可重入的变量
void gpio_hal_input_enable(uint8_t gpio_id, uint8_t gpio_num){
    
    ddr |= (1 << gpio_num);
}


void gpio_hal_output_enable(uint8_t gpio_id, uint8_t gpio_num){

    ddr &= ~(1 << gpio_num);
}


void gpio_hal_set_level(uint8_t gpio_id, uint8_t gpio_num, uint8_t level){
    if (level == GPIO_LEVEL_HIGH)
    {
        wdr |= (1 << gpio_num);
    }
    else
    {
        wdr &= ~(1 << gpio_num);
    }
}

uint8_t gpio_hal_get_level(uint8_t gpio_id, uint8_t gpio_num){
    uint8_t is_input_pin = (ddr>>gpio_num) & 0x01;
    if(is_input_pin) return (rdr & (1 << gpio_num)) ? 1 : 0;
    else return (wdr & (1 << gpio_num)) ? 1 : 0;
}

void gpio_hal_read_update(){
    rdr = REG_GPIO_0_DR;
#ifndef CONFIG_DRIVER_GPIO_OVERCLOCK
    hal_delay_us(0,50);
#endif
}

void gpio_hal_write_update(){
    REG_GPIO_0_DDR = ddr;
    REG_GPIO_0_DR = wdr;
#ifndef CONFIG_DRIVER_GPIO_OVERCLOCK
    hal_delay_us(0,50);
#endif
}

void gpio_hal_set_fcfg(uint8_t gpio_id, uint8_t gpio_num, uint8_t val){
    // StarrySkyC2 does not implement FCFG
    assert(0,"[Warn] StarrySkyC2 does not implement FCFG\n");
}

void gpio_hal_set_mux(uint8_t gpio_id, uint8_t gpio_num, uint8_t val){
    // StarrySkyC2 does not implement PINMUX
    assert(0,"[Warn] StarrySkyC2 does not implement PINMUX\n");
}