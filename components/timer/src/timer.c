#include "timer.h"
#include "stdio.h"
#include "generated/autoconf.h"
#include "board.h"

void delay_us(uint32_t val){
#if CONFIG_TIMER_IP_ID == 0
    REG_TIM_0_CONFIG = (uint32_t)0x0100;
    REG_TIM_0_DATA = (uint32_t)(CONFIG_CPU_FREQ_MHZ * val - 1);
    REG_TIM_0_CONFIG = (uint32_t)0x0101; // irq disable, count down, continuous mode, timer enable
    while(REG_TIM_0_DATA != 0)
        ;
#elif CONFIG_TIMER_IP_ID == 1

#endif
}

void delay_ms(uint32_t val){
    delay_us(val * 1000);
}

void delay_s(uint32_t val){
    delay_ms(val * 1000);
}

void sys_tick_init(void){
#if CONFIG_TIMER_IP_ID == 0
    REG_TIM_1_CONFIG = (uint32_t)0x0100;
    REG_TIM_1_DATA = (uint32_t)0xFFFFFFFF;
    REG_TIM_1_CONFIG = (uint32_t)0x0101; // irq disable, count up, continuous mode, timer enable
#elif CONFIG_TIMER_IP_ID == 1

#endif
}

uint32_t get_sys_tick(void){
#if CONFIG_TIMER_IP_ID == 0
    return 0xFFFFFFFF - REG_TIM_1_DATA;
#elif CONFIG_TIMER_IP_ID == 1
    return 0;
#endif
}
