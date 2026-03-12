/**
 * @file pwm.h
 * @brief PWM Hardware Abstraction Layer interface
 */

#ifndef PWM_H__
#define PWM_H__

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PWM port number type
 */
typedef uint32_t pwm_port_t;

/**
 * @brief PWM channel enumeration
 */
typedef enum {
    PWM_CH0 = 0,    /**< PWM Channel 0 */
    PWM_CH1 = 1,    /**< PWM Channel 1 */
    PWM_CH2 = 2,    /**< PWM Channel 2 */
    PWM_CH3 = 3     /**< PWM Channel 3 */
} pwm_channel_t;

/**
 * @brief PWM configuration structure
 */
typedef struct {
    uint32_t prescaler;     /**< Prescaler value */
    uint32_t compare_value; /**< Compare value */
} pwm_config_t;

/**
 * @brief PWM driver interface structure
 */
typedef struct {
    driver_base_t base;     /**< Base driver structure */
    
    /**
     * @brief Initialize PWM with configuration
     * @param port PWM port number
     * @param config Pointer to PWM configuration
     * @return Status code
     */
    status_t (*init)(pwm_port_t port, const pwm_config_t* config);
    
    /**
     * @brief Set PWM compare value for a channel
     * @param port PWM port number
     * @param channel PWM channel
     * @param compare_value Compare value
     * @return Status code
     */
    status_t (*set_compare)(pwm_port_t port, pwm_channel_t channel, uint32_t compare_value);
    
} pwm_driver_t;

/**
 * @brief Register a PWM driver
 * @param driver Pointer to the PWM driver to register
 * @return Status code
 */
status_t pwm_register_driver(const pwm_driver_t* driver);

/**
 * @brief Get the currently registered PWM driver
 * @return Pointer to the active PWM driver, or NULL if none registered
 */
pwm_driver_t* pwm_get_driver(void);

#ifdef __cplusplus
}
#endif

#endif /* PWM_H__ */