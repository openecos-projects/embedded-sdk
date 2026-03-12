/**
 * @file gpio.h
 * @brief GPIO Hardware Abstraction Layer interface
 */

#ifndef GPIO_H__
#define GPIO_H__

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO pin number type
 */
typedef uint32_t gpio_pin_t;

/**
 * @brief GPIO mode enumeration
 */
typedef enum {
    GPIO_MODE_INPUT = 0,    /**< Input mode */
    GPIO_MODE_OUTPUT = 1    /**< Output mode */
} gpio_mode_t;

/**
 * @brief GPIO level enumeration
 */
typedef enum {
    GPIO_LEVEL_LOW = 0,     /**< Low level */
    GPIO_LEVEL_HIGH = 1     /**< High level */
} gpio_level_t;

/**
 * @brief GPIO function enumeration
 */
typedef enum {
    GPIO_FUNC_GPIO = 0,     /**< GPIO function */
    GPIO_FUNC_MUX_0 = 1,    /**< Multiplexer function 0 */
    GPIO_FUNC_MUX_1 = 2     /**< Multiplexer function 1 */
} gpio_func_t;

/**
 * @brief GPIO configuration structure
 */
typedef struct {
    uint64_t pin_bit_mask;      /**< Pin bit mask for multiple pins */
    gpio_mode_t mode;           /**< GPIO mode */
} gpio_config_t;

/**
 * @brief GPIO driver interface structure
 * 
 * This structure defines the standard interface that all GPIO drivers must implement.
 */
typedef struct {
    driver_base_t base;         /**< Base driver structure */
    
    /**
     * @brief Configure GPIO pins
     * @param config Pointer to GPIO configuration
     * @return Status code
     */
    status_t (*config)(const gpio_config_t* config);
    
    /**
     * @brief Set GPIO pin direction
     * @param pin GPIO pin number
     * @param mode GPIO mode
     * @return Status code
     */
    status_t (*set_direction)(gpio_pin_t pin, gpio_mode_t mode);
    
    /**
     * @brief Set GPIO pin level
     * @param pin GPIO pin number
     * @param level GPIO level
     * @return Status code
     */
    status_t (*set_level)(gpio_pin_t pin, gpio_level_t level);
    
    /**
     * @brief Get GPIO pin level
     * @param pin GPIO pin number
     * @param level Pointer to store the read level
     * @return Status code
     */
    status_t (*get_level)(gpio_pin_t pin, gpio_level_t* level);
    
    /**
     * @brief Set GPIO pin function/multiplexer
     * @param pin GPIO pin number
     * @param func GPIO function
     * @return Status code
     */
    status_t (*set_function)(gpio_pin_t pin, gpio_func_t func);
    
} gpio_driver_t;

/**
 * @brief Register a GPIO driver
 * 
 * This function registers a GPIO driver with the HAL system.
 * Only one GPIO driver can be active at a time.
 * 
 * @param driver Pointer to the GPIO driver to register
 * @return Status code
 */
status_t gpio_register_driver(const gpio_driver_t* driver);

/**
 * @brief Get the currently registered GPIO driver
 * 
 * @return Pointer to the active GPIO driver, or NULL if none registered
 */
gpio_driver_t* gpio_get_driver(void);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H__ */