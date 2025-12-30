#include "pwm.h"
#include "board.h"

void pwm_init(pwm_config_t* config){
#if CONFIG_PWM_IP_ID == 0
    REG_PWM_0_PSCR = config->pscr;
    REG_PWM_0_CMP = config->cmp;
    REG_PWM_0_CTRL = 3;
#elif CONFIG_PWM_IP_ID == 1

#endif
}

void pwm_set_compare(pwm_channel_t ch, uint32_t cmp){
#if CONFIG_PWM_IP_ID == 0
    switch(ch){
        case PWM_CH0:
            REG_PWM_0_CR0 = cmp;
            break;
        case PWM_CH1:
            REG_PWM_0_CR1 = cmp;
            break;
        case PWM_CH2:
            REG_PWM_0_CR2 = cmp;
            break;
        case PWM_CH3:
            REG_PWM_0_CR3 = cmp;
            break;
    }
#elif CONFIG_PWM_IP_ID == 1

#endif
}

