/**
 * @file pwm_driver.h
 * @brief StarrySky C2 PWM driver header
 */

#ifndef PWM_DRIVER_H__
#define PWM_DRIVER_H__

#include "pwm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize StarrySky C2 PWM driver
 * @return Status code
 */
status_t starrysky_c2_pwm_init(void);

#ifdef __cplusplus
}
#endif

#endif /* PWM_DRIVER_H__ */