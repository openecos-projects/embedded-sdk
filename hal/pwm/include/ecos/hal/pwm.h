#ifndef ECOS_HAL_PWM_H
#define ECOS_HAL_PWM_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t hal_pwm_id_t;

typedef enum {
    HAL_PWM_CHANNEL_0 = 0,
    HAL_PWM_CHANNEL_1,
    HAL_PWM_CHANNEL_2,
    HAL_PWM_CHANNEL_3,
    HAL_PWM_CHANNEL_COUNT
} hal_pwm_channel_t;

typedef struct {
    uint32_t clock_divider;
    uint32_t period_ticks;
} hal_pwm_config_t;

int hal_pwm_get_instance_count(void);
ecos_err_t hal_pwm_init(hal_pwm_id_t pwm, const hal_pwm_config_t *config);
ecos_err_t hal_pwm_set_duty_cycle(hal_pwm_id_t pwm,
                                  hal_pwm_channel_t channel,
                                  uint8_t duty_percent);
ecos_err_t hal_pwm_start(hal_pwm_id_t pwm);
ecos_err_t hal_pwm_stop(hal_pwm_id_t pwm);

#ifdef __cplusplus
}
#endif

#endif
