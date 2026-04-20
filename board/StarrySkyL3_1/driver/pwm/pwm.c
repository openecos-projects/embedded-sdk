#include "hal_pwm.h"
#include "hal_pwm_type.h"
#include "board.h"
#include <stdint.h>

uint8_t pwm_hal_init(void *hal, uint8_t timer_id, pwm_config_t *config){
    if (timer_id == 0)
    {
        REG_PWM_0_CTRL = 0;
        REG_PWM_0_PSCR = config->pscr;
        REG_PWM_0_CMP  = config->cmp;
    }   
    else if (timer_id == 1)
    {
        REG_PWM_1_CTRL = 0;
        REG_PWM_1_PSCR = config->pscr;
        REG_PWM_1_CMP  = config->cmp;
    }
    else if (timer_id == 2)
    {
        REG_PWM_2_CTRL = 0;
        REG_PWM_2_PSCR = config->pscr;
        REG_PWM_2_CMP  = config->cmp;
    }
    return 0;
}

uint8_t pwm_hal_set_compare(void *hal, uint8_t timer_id, pwm_channel_t ch, uint32_t cmp){
    if (timer_id == 0)
    {
        switch(ch)
        {
            case PWM_CH0: REG_PWM_0_CR0 = cmp; break;
            case PWM_CH1: REG_PWM_0_CR1 = cmp; break;
            case PWM_CH2: REG_PWM_0_CR2 = cmp; break;
            case PWM_CH3: REG_PWM_0_CR3 = cmp; break;
            default: break;
        }
    }   
    else if (timer_id == 1)
    {
        switch(ch)
        {
            case PWM_CH0: REG_PWM_1_CR0 = cmp; break;
            case PWM_CH1: REG_PWM_1_CR1 = cmp; break;
            case PWM_CH2: REG_PWM_1_CR2 = cmp; break;
            case PWM_CH3: REG_PWM_1_CR3 = cmp; break;
            default: break;
        }
    }
    else if (timer_id == 2)
    {
        switch(ch)
        {
            case PWM_CH0: REG_PWM_2_CR0 = cmp; break;
            case PWM_CH1: REG_PWM_2_CR1 = cmp; break;
            case PWM_CH2: REG_PWM_2_CR2 = cmp; break;
            case PWM_CH3: REG_PWM_2_CR3 = cmp; break;
            default: break;
        }
    }
    return 0;
}

uint8_t pwm_hal_enable(void *hal, uint8_t timer_id){
    if (timer_id == 0)
    {
        REG_PWM_0_CTRL = 3;
    }   
    else if (timer_id == 1)
    {
        REG_PWM_1_CTRL = 3;
    }
    else if (timer_id == 2)
    {
        REG_PWM_2_CTRL = 3;
    }
    return 0;
}

uint8_t pwm_hal_disable(void *hal, uint8_t timer_id){
    if (timer_id == 0)
    {
        REG_PWM_0_CTRL = 0;
    }   
    else if (timer_id == 1)
    {
        REG_PWM_1_CTRL = 0;
    }
    else if (timer_id == 2)
    {
        REG_PWM_2_CTRL = 0;
    }
    return 0;
}
