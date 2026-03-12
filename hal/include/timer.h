/**
 * @file hal_timer.h
 * @brief Timer Hardware Abstraction Layer interface
 */

#ifndef HAL_TIMER_H__
#define HAL_TIMER_H__

#include <stdint.h>
#include "hal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Timer ID type
 */
typedef uint32_t hal_timer_id_t;

/**
 * @brief Timer mode enumeration
 */
typedef enum {
    HAL_TIMER_MODE_ONESHOT = 0,     /**< One-shot timer */
    HAL_TIMER_MODE_PERIODIC = 1     /**< Periodic timer */
} hal_timer_mode_t;

/**
 * @brief Timer configuration structure
 */
typedef struct {
    hal_timer_mode_t mode;          /**< Timer mode */
    uint32_t period_us;             /**< Timer period in microseconds */
    hal_interrupt_callback_t callback; /**< Callback function (can be NULL) */
    void* callback_arg;             /**< Argument for callback function */
} hal_timer_config_t;

/**
 * @brief Timer driver interface structure
 */
typedef struct {
    hal_driver_base_t base;         /**< Base driver structure */
    
    /**
     * @brief Initialize timer with configuration
     * @param timer_id Timer ID
     * @param config Pointer to timer configuration
     * @return HAL status code
     */
    hal_status_t (*init)(hal_timer_id_t timer_id, const hal_timer_config_t* config);
    
    /**
     * @brief Deinitialize timer
     * @param timer_id Timer ID
     * @return HAL status code
     */
    hal_status_t (*deinit)(hal_timer_id_t timer_id);
    
    /**
     * @brief Start timer
     * @param timer_id Timer ID
     * @return HAL status code
     */
    hal_status_t (*start)(hal_timer_id_t timer_id);
    
    /**
     * @brief Stop timer
     * @param timer_id Timer ID
     * @return HAL status code
     */
    hal_status_t (*stop)(hal_timer_id_t timer_id);
    
    /**
     * @brief Get current timer value
     * @param timer_id Timer ID
     * @param value Pointer to store current timer value
     * @return HAL status code
     */
    hal_status_t (*get_value)(hal_timer_id_t timer_id, uint32_t* value);
    
    /**
     * @brief Set timer period
     * @param timer_id Timer ID
     * @param period_us New period in microseconds
     * @return HAL status code
     */
    hal_status_t (*set_period)(hal_timer_id_t timer_id, uint32_t period_us);
    
} hal_timer_driver_t;

/**
 * @brief Register a timer driver
 * @param driver Pointer to the timer driver to register
 * @return HAL status code
 */
hal_status_t hal_timer_register_driver(const hal_timer_driver_t* driver);

/**
 * @brief Get the currently registered timer driver
 * @return Pointer to the active timer driver, or NULL if none registered
 */
hal_timer_driver_t* hal_timer_get_driver(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_TIMER_H__ */