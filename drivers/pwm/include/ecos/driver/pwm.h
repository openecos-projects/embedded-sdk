#ifndef ECOS_DRIVER_PWM_H
#define ECOS_DRIVER_PWM_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ecos_pwm_id_t;

#define ECOS_PWM_DEFAULT ((ecos_pwm_id_t)0u)

typedef enum {
    ECOS_PWM_CHANNEL_0 = 0,
    ECOS_PWM_CHANNEL_1,
    ECOS_PWM_CHANNEL_2,
    ECOS_PWM_CHANNEL_3,
    ECOS_PWM_CHANNEL_COUNT
} ecos_pwm_channel_t;

typedef struct {
    uint32_t clock_divider;
    uint32_t period_ticks;
} ecos_pwm_config_t;

#define ECOS_PWM_CONFIG_DEFAULT \
    { 1u, 1000u }

/* Returns the number of PWM controller instances provided by the Target. */
int ecos_pwm_get_instance_count(void);

/* Configure a controller without starting its counter. */
ecos_err_t ecos_pwm_init(ecos_pwm_id_t pwm,
                         const ecos_pwm_config_t *config);

/* Configure one channel's duty cycle in the inclusive range 0..100 percent. */
ecos_err_t ecos_pwm_set_duty_cycle(ecos_pwm_id_t pwm,
                                   ecos_pwm_channel_t channel,
                                   uint8_t duty_percent);

ecos_err_t ecos_pwm_start(ecos_pwm_id_t pwm);
ecos_err_t ecos_pwm_stop(ecos_pwm_id_t pwm);

#ifdef __cplusplus
}
#endif

#endif
