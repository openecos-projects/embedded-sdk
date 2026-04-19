#include "hal_timer.h"
#include "generated/autoconf.h"
#include "board.h"

// 提供一个弱定义的延时函数，供 devices 外设驱动默认调用。
// 用户可以在自己的应用代码（如 main.c）中重写这个函数，提供更精准的实现。
__attribute__((weak)) void delay_ms(uint32_t val) {
    hal_delay_ms(0, val);
}

uint8_t hal_delay_us(uint8_t timer_id, uint32_t val){
    if (timer_id == 0){
        REG_TIMER_0_CTRL = 0;
        while (REG_TIMER_0_STAT == 1)
            ; // clear irq
        REG_TIMER_0_PSCR = CONFIG_TIMER_FREQ_MHZ - 1;
        REG_TIMER_0_CMP = val - 1;
        REG_TIMER_0_CTRL = 0xD; // enable timer
        while(REG_TIMER_0_STAT == 0)
            ;
        REG_TIMER_0_CTRL = 0; // disable timer
        while (REG_TIMER_0_STAT == 1)
            ; // clear irq
        return 0;
    }else{
        return 1;
    }
}

uint8_t hal_delay_ms(uint8_t timer_id, uint32_t val){
    if (timer_id == 0){
        REG_TIMER_0_CTRL = 0;
        while (REG_TIMER_0_STAT == 1)
            ; // clear irq
        
        // 1 count = 1us
        REG_TIMER_0_PSCR = CONFIG_TIMER_FREQ_MHZ - 1;
        // Total counts = val * 1000 us
        REG_TIMER_0_CMP = (val * 1000) - 1; 
        
        REG_TIMER_0_CTRL = 0xD; // enable timer
        while(REG_TIMER_0_STAT == 0)
            ;
            
        REG_TIMER_0_CTRL = 0; // disable timer
        while (REG_TIMER_0_STAT == 1)
            ; // clear irq
        return 0;
    }else{
        return 1;
    }
}

uint8_t hal_delay_s(uint8_t timer_id, uint32_t val){
    return hal_delay_ms(timer_id, val * 1000);
}

uint8_t hal_sys_tick_init(uint8_t timer_id){
    if (timer_id == 0){
        REG_TIMER_0_CTRL = 0;
        while (REG_TIMER_0_STAT == 1)
            ; // clear irq
        REG_TIMER_0_PSCR = CONFIG_TIMER_FREQ_MHZ - 1;
        REG_TIMER_0_CMP = 0xFFFFFFFF;
        REG_TIMER_0_CTRL = 0xD; // enable timer
        return 0;
    }else{
        return 1;
    }
}

uint32_t hal_get_sys_tick(uint8_t timer_id){
    if (timer_id == 0){
        return REG_TIMER_0_CNT;
    }else{
        return 0;
    }
}
