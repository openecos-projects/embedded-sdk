/**
 * @file hal_gpio.c
 * @brief GPIO Hardware Abstraction Layer implementation
 */

#include "hal_gpio.h"
#include <stddef.h>

// Static variable to store the registered GPIO driver
static hal_gpio_driver_t* g_hal_gpio_driver = NULL;

hal_status_t hal_gpio_register_driver(const hal_gpio_driver_t* driver)
{
    // Check if driver pointer is valid
    HAL_CHECK_NULL(driver);
    
    // Check if all required function pointers are implemented
    if (driver->config == NULL || 
        driver->set_direction == NULL || 
        driver->set_level == NULL || 
        driver->get_level == NULL || 
        driver->set_function == NULL) {
        return HAL_INVALID_ARG;
    }
    
    // Register the driver
    g_hal_gpio_driver = (hal_gpio_driver_t*)driver;
    
    return HAL_SUCCESS;
}

hal_gpio_driver_t* hal_gpio_get_driver(void)
{
    return g_hal_gpio_driver;
}