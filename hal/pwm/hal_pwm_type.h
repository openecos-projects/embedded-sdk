#ifndef __HAL_PWM_TYPE_H__
#define __HAL_PWM_TYPE_H__

#include <stdint.h>


typedef enum{
    PWM_CH0 = 0,  
    PWM_CH1 = 1,  
    PWM_CH2 = 2,  
    PWM_CH3 = 3, 
    PWM_CH_MAX    
} pwm_channel_t;

typedef struct {
    uint32_t pscr; 
    uint32_t cmp;  
} pwm_config_t;

typedef enum {
    PWM_STATE_DISABLE = 0,  
    PWM_STATE_ENABLE  = 1  
} pwm_state_t;

#endif 