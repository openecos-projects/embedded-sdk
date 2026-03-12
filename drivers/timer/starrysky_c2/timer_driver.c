/**
 * @file timer_driver.c
 * @brief StarrySky C2 Timer driver implementation
 */

#include "timer.h"
#include "board.h"  // Include C2 board header for register definitions

// Static function declarations
static status_t timer_init_impl(timer_id_t timer_id, const timer_config_t* config);
static status_t timer_deinit_impl(timer_id_t timer_id);
static status_t timer_start_impl(timer_id_t timer_id);
static status_t timer_stop_impl(timer_id_t timer_id);
static status_t timer_get_value_impl(timer_id_t timer_id, uint32_t* value);
static status_t timer_set_period_impl(timer_id_t timer_id, uint32_t period_us);

// Timer driver instance for StarrySky C2
static timer_driver_t starrysky_c2_timer_driver = {
    .base = {
        .version = DRIVER_VERSION(1, 0),
        .name = "starrysky_c2_timer"
    },
    .init = timer_init_impl,
    .deinit = timer_deinit_impl,
    .start = timer_start_impl,
    .stop = timer_stop_impl,
    .get_value = timer_get_value_impl,
    .set_period = timer_set_period_impl
};

status_t starrysky_c2_timer_init(void)
{
    return timer_register_driver(&starrysky_c2_timer_driver);
}

// Helper function to convert microseconds to timer ticks
// Assuming system clock frequency is known (e.g., 50MHz)
static uint32_t us_to_ticks(uint32_t us)
{
    // For 50MHz clock: 1 tick = 20ns, so 1us = 50 ticks
    // Adjust this based on actual system clock
    return us * 50; // Example conversion
}

static status_t timer_init_impl(timer_id_t timer_id, const timer_config_t* config)
{
    CHECK_NULL(config);
    
    // Validate timer ID (C2 has Timer 0 and Timer 1)
    if (timer_id > 1) {
        return STATUS_INVALID_ARG;
    }
    
    uint32_t period_ticks = us_to_ticks(config->period_us);
    
    if (timer_id == 0) {
        // Configure Timer 0
        REG_TIM_0_DATA = period_ticks; // Set reload value
        REG_TIM_0_CONFIG = 0x01;       // Enable timer (bit 0)
        
        // Store callback info for interrupt handling (if needed)
        // For now, we'll assume polling mode
    } else {
        // Configure Timer 1  
        REG_TIM_1_DATA = period_ticks; // Set reload value
        REG_TIM_1_CONFIG = 0x01;       // Enable timer (bit 0)
    }
    
    return STATUS_SUCCESS;
}

static status_t timer_deinit_impl(timer_id_t timer_id)
{
    // Validate timer ID
    if (timer_id > 1) {
        return STATUS_INVALID_ARG;
    }
    
    if (timer_id == 0) {
        REG_TIM_0_CONFIG = 0x00; // Disable timer
    } else {
        REG_TIM_1_CONFIG = 0x00; // Disable timer
    }
    
    return STATUS_SUCCESS;
}

static status_t timer_start_impl(timer_id_t timer_id)
{
    // Validate timer ID
    if (timer_id > 1) {
        return STATUS_INVALID_ARG;
    }
    
    // Timer is already running after init, but we can restart it
    if (timer_id == 0) {
        REG_TIM_0_CONFIG |= 0x01; // Ensure enabled
    } else {
        REG_TIM_1_CONFIG |= 0x01; // Ensure enabled
    }
    
    return STATUS_SUCCESS;
}

static status_t timer_stop_impl(timer_id_t timer_id)
{
    // Validate timer ID
    if (timer_id > 1) {
        return STATUS_INVALID_ARG;
    }
    
    if (timer_id == 0) {
        REG_TIM_0_CONFIG &= ~0x01; // Disable timer
    } else {
        REG_TIM_1_CONFIG &= ~0x01; // Disable timer
    }
    
    return STATUS_SUCCESS;
}

static status_t timer_get_value_impl(timer_id_t timer_id, uint32_t* value)
{
    CHECK_NULL(value);
    
    // Validate timer ID
    if (timer_id > 1) {
        return STATUS_INVALID_ARG;
    }
    
    if (timer_id == 0) {
        *value = REG_TIM_0_VALUE; // Read current counter value
    } else {
        *value = REG_TIM_1_VALUE; // Read current counter value
    }
    
    return STATUS_SUCCESS;
}

static status_t timer_set_period_impl(timer_id_t timer_id, uint32_t period_us)
{
    // Validate timer ID
    if (timer_id > 1) {
        return STATUS_INVALID_ARG;
    }
    
    uint32_t period_ticks = us_to_ticks(period_us);
    
    if (timer_id == 0) {
        REG_TIM_0_DATA = period_ticks; // Update reload value
    } else {
        REG_TIM_1_DATA = period_ticks; // Update reload value
    }
    
    return STATUS_SUCCESS;
}