#ifndef __PWM_H__
#define __PWM_H__

#include <stdint.h>
#include "pwm_type.h"

void pwm_init(pwm_config_t* config);
void pwm_set_compare(pwm_channel_t ch, uint32_t cmp);

#endif
