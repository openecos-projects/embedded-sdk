/**
 * @file gpio_driver.c
 * @brief StarrySky C2 GPIO driver implementation
 */

#include "gpio.h"
#include "board.h"  // Include C2 board header for register definitions

// Static function declarations
static status_t gpio_config_impl(const gpio_config_t* config);
static status_t gpio_set_direction_impl(gpio_pin_t pin, gpio_mode_t mode);
static status_t gpio_set_level_impl(gpio_pin_t pin, gpio_level_t level);
static status_t gpio_get_level_impl(gpio_pin_t pin, gpio_level_t* level);
static status_t gpio_set_function_impl(gpio_pin_t pin, gpio_func_t func);

// GPIO driver instance for StarrySky C2
static gpio_driver_t starrysky_c2_gpio_driver = {
    .base = {
        .version = DRIVER_VERSION(1, 0),
        .name = "starrysky_c2_gpio"
    },
    .config = gpio_config_impl,
    .set_direction = gpio_set_direction_impl,
    .set_level = gpio_set_level_impl,
    .get_level = gpio_get_level_impl,
    .set_function = gpio_set_function_impl
};

status_t starrysky_c2_gpio_init(void)
{
    return gpio_register_driver(&starrysky_c2_gpio_driver);
}

static status_t gpio_config_impl(const gpio_config_t* config)
{
    CHECK_NULL(config);
    
    uint64_t pin_bit_mask = config->pin_bit_mask;
    uint32_t pin_num = 0;
    
    // Iterate through all possible pins (0-31 for C2)
    while (pin_num < 32) {
        if ((pin_bit_mask >> pin_num) & 0x01) {
            if (config->mode == GPIO_MODE_INPUT) {
                // Set as input: set DDR bit to 1
                REG_GPIO_0_DDR |= (1U << pin_num);
            } else {
                // Set as output: clear DDR bit to 0
                REG_GPIO_0_DDR &= ~(1U << pin_num);
            }
        }
        pin_num++;
    }
    
    return STATUS_SUCCESS;
}

static status_t gpio_set_direction_impl(gpio_pin_t pin, gpio_mode_t mode)
{
    // Validate pin number (C2 has GPIO 0-31)
    if (pin >= 32) {
        return STATUS_INVALID_ARG;
    }
    
    if (mode == GPIO_MODE_INPUT) {
        // Set as input: set DDR bit to 1
        REG_GPIO_0_DDR |= (1U << pin);
    } else {
        // Set as output: clear DDR bit to 0
        REG_GPIO_0_DDR &= ~(1U << pin);
    }
    
    return STATUS_SUCCESS;
}

static status_t gpio_set_level_impl(gpio_pin_t pin, gpio_level_t level)
{
    // Validate pin number (C2 has GPIO 0-31)
    if (pin >= 32) {
        return STATUS_INVALID_ARG;
    }
    
    if (level == GPIO_LEVEL_HIGH) {
        // Set high: set DR bit to 1
        REG_GPIO_0_DR |= (1U << pin);
    } else {
        // Set low: clear DR bit to 0
        REG_GPIO_0_DR &= ~(1U << pin);
    }
    
    return STATUS_SUCCESS;
}

static status_t gpio_get_level_impl(gpio_pin_t pin, gpio_level_t* level)
{
    CHECK_NULL(level);
    
    // Validate pin number (C2 has GPIO 0-31)
    if (pin >= 32) {
        return STATUS_INVALID_ARG;
    }
    
    // Read the current level from DR register
    if (REG_GPIO_0_DR & (1U << pin)) {
        *level = GPIO_LEVEL_HIGH;
    } else {
        *level = GPIO_LEVEL_LOW;
    }
    
    return STATUS_SUCCESS;
}

static status_t gpio_set_function_impl(gpio_pin_t pin, gpio_func_t func)
{
    // C2 GPIO doesn't have multiplexer functionality
    // All pins are dedicated GPIO, so this function is a no-op
    // but we still validate the pin number
    
    if (pin >= 32) {
        return STATUS_INVALID_ARG;
    }
    
    // For C2, GPIO_FUNC_GPIO (0) is the only valid option
    if (func != GPIO_FUNC_GPIO) {
        return STATUS_NOT_SUPPORTED;
    }
    
    return STATUS_SUCCESS;
}