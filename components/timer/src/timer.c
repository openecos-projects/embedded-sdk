#include "timer.h"
#include "stdio.h"
#include "generated/autoconf.h"
#include "board.h"

void delay_us(uint32_t val){
#ifdef CONFIG_STARRYSKY_C1
    REG_TIM_0_CONFIG = (uint32_t)0x0100;
    REG_TIM_0_DATA = (uint32_t)(CONFIG_CPU_FREQ_MHZ * val - 1);
    REG_TIM_0_CONFIG = (uint32_t)0x0101; // irq disable, count down, continuous mode, timer enable
    while(REG_TIM_0_DATA != 0)
        ;
#elif CONFIG_STARRYSKY_L3

#endif
}

void delay_ms(uint32_t val){
    delay_us(val * 1000);
}

void delay_s(uint32_t val){
    delay_ms(val * 1000);
}

void sys_tick_init(void){
#ifdef CONFIG_STARRYSKY_C1
    REG_TIM_1_CONFIG = (uint32_t)0x0100;
    REG_TIM_1_DATA = (uint32_t)0xFFFFFFFF;
    REG_TIM_1_CONFIG = (uint32_t)0x0101; // irq disable, count up, continuous mode, timer enable
#elif CONFIG_STARRYSKY_L3

#endif
}

uint32_t get_sys_tick(void){
#ifdef CONFIG_STARRYSKY_C1
    return 0xFFFFFFFF - REG_TIM_1_DATA;
#elif CONFIG_STARRYSKY_L3
    return 0;
#endif
}
