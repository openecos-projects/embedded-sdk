#include "hal_timer.h"
#include "generated/autoconf.h"
#include "board.h"

uint8_t hal_delay_us(uint8_t timer_id, uint32_t val){
    if (timer_id == 0){
        REG_TIM_0_CONFIG = (uint32_t)0x0100;
        REG_TIM_0_DATA = (uint32_t)(CONFIG_CPU_FREQ_MHZ * val - 1);
        REG_TIM_0_CONFIG = (uint32_t)0x0101;
        while(REG_TIM_0_DATA != 0)
            ;
        return 0;
    }else if (timer_id == 1){
        REG_TIM_1_CONFIG = (uint32_t)0x0100;
        REG_TIM_1_DATA = (uint32_t)(CONFIG_CPU_FREQ_MHZ * val - 1);
        REG_TIM_1_CONFIG = (uint32_t)0x0101;
        while(REG_TIM_1_DATA != 0)
            ;
        return 0;
    }else{
        return 1;
    }
}
uint8_t hal_delay_ms(uint8_t timer_id, uint32_t val){
    return hal_delay_us(timer_id, val * 1000);
}
uint8_t hal_delay_s(uint8_t timer_id, uint32_t val){
    return hal_delay_ms(timer_id, val * 1000);
}
uint8_t hal_sys_tick_init(uint8_t timer_id){
    if (timer_id == 0){
        REG_TIM_0_CONFIG = (uint32_t)0x0100;
        REG_TIM_0_DATA = (uint32_t)0xFFFFFFFF;
        REG_TIM_0_CONFIG = (uint32_t)0x0101;
        return 0;
    }else if (timer_id == 1){
        REG_TIM_1_CONFIG = (uint32_t)0x0100;
        REG_TIM_1_DATA = (uint32_t)0xFFFFFFFF;
        REG_TIM_1_CONFIG = (uint32_t)0x0101;
        return 0;
    }else{
        return 1;
    }
}

uint32_t hal_get_sys_tick(uint8_t timer_id){
    if (timer_id == 0){
        return 0xFFFFFFFF - REG_TIM_0_DATA;
    }else if (timer_id == 1){
        return 0xFFFFFFFF - REG_TIM_1_DATA;
    }else{
        return 0;
    }
}
