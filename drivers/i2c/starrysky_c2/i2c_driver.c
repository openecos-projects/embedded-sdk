/**
 * @file i2c_driver.c
 * @brief StarrySky C2 I2C driver implementation
 */

#include "i2c.h"
#include "board.h"  // Include C2 board header for register definitions
#include <string.h>

// Static function declarations
static status_t i2c_init_impl(i2c_port_t port, const i2c_config_t* config);
static status_t i2c_deinit_impl(i2c_port_t port);
static status_t i2c_master_write_impl(i2c_port_t port, uint32_t slave_addr, 
                                    const uint8_t* data, size_t length, uint32_t timeout_ms);
static status_t i2c_master_read_impl(i2c_port_t port, uint32_t slave_addr, 
                                   uint8_t* data, size_t length, uint32_t timeout_ms);
static status_t i2c_master_write_read_impl(i2c_port_t port, uint32_t slave_addr,
                                        const uint8_t* write_data, size_t write_length,
                                        uint8_t* read_data, size_t read_length, 
                                        uint32_t timeout_ms);
static status_t i2c_is_bus_busy_impl(i2c_port_t port, bool_t* busy);

// I2C driver instance for StarrySky C2
static i2c_driver_t starrysky_c2_i2c_driver = {
    .base = {
        .version = DRIVER_VERSION(1, 0),
        .name = "starrysky_c2_i2c"
    },
    .init = i2c_init_impl,
    .deinit = i2c_deinit_impl,
    .master_write = i2c_master_write_impl,
    .master_read = i2c_master_read_impl,
    .master_write_read = i2c_master_write_read_impl,
    .is_bus_busy = i2c_is_bus_busy_impl
};

status_t starrysky_c2_i2c_init(void)
{
    return i2c_register_driver(&starrysky_c2_i2c_driver);
}

static status_t i2c_init_impl(i2c_port_t port, const i2c_config_t* config)
{
    CHECK_NULL(config);
    
    // C2 only supports I2C port 0
    if (port != 0) {
        return STATUS_INVALID_ARG;
    }
    
    // Validate address width (C2 only supports 7-bit)
    if (config->addr_width != I2C_ADDR_7BIT) {
        return STATUS_NOT_SUPPORTED;
    }
    
    // Validate speed (C2 supports standard and fast mode)
    if (config->speed != I2C_SPEED_STANDARD && 
        config->speed != I2C_SPEED_FAST) {
        return STATUS_NOT_SUPPORTED;
    }
    
    // Calculate prescaler value based on desired speed
    // Assuming system clock is known, calculate PSCR value
    uint32_t pscr_value = 0;
    switch (config->speed) {
        case I2C_SPEED_STANDARD:  // 100kHz
            pscr_value = 49;  // Example value, adjust based on actual clock
            break;
        case I2C_SPEED_FAST:      // 400kHz  
            pscr_value = 12;  // Example value, adjust based on actual clock
            break;
        default:
            return STATUS_INVALID_ARG;
    }
    
    // Configure I2C controller
    REG_I2C_0_PSCR = pscr_value;  // Set prescaler
    REG_I2C_0_CTRL = 0x80;        // Enable I2C controller
    
    return STATUS_SUCCESS;
}

static status_t i2c_deinit_impl(i2c_port_t port)
{
    if (port != 0) {
        return STATUS_INVALID_ARG;
    }
    
    // Disable I2C controller
    REG_I2C_0_CTRL = 0x00;
    
    return STATUS_SUCCESS;
}

static status_t i2c_wait_for_completion(uint32_t timeout_ms)
{
    uint32_t timeout_count = timeout_ms * 1000;  // Convert to microsecond count
    
    while (!(REG_I2C_0_SR & 0x01) && timeout_count > 0) {
        // Wait for transfer complete bit
        timeout_count--;
        // Simple delay - in real implementation, use proper timing
        volatile int delay = 100;
        while (delay--);
    }
    
    if (timeout_count == 0) {
        return STATUS_TIMEOUT;
    }
    
    return STATUS_SUCCESS;
}

static status_t i2c_master_write_impl(i2c_port_t port, uint32_t slave_addr, 
                                    const uint8_t* data, size_t length, uint32_t timeout_ms)
{
    CHECK_NULL(data);
    
    if (port != 0 || length == 0) {
        return STATUS_INVALID_ARG;
    }
    
    // Validate 7-bit address
    if (slave_addr > 0x7F) {
        return STATUS_INVALID_ARG;
    }
    
    // Send START condition + slave address + write bit (0)
    REG_I2C_0_TXR = (slave_addr << 1) | 0x00;  // Write bit = 0
    REG_I2C_0_CMD = 0x90;  // START + WRITE
    
    // Wait for address phase completion
    status_t ret = i2c_wait_for_completion(timeout_ms);
    if (ret != STATUS_SUCCESS) {
        return ret;
    }
    
    // Send data bytes
    for (size_t i = 0; i < length; i++) {
        REG_I2C_0_TXR = data[i];
        REG_I2C_0_CMD = 0x10;  // WRITE
        
        ret = i2c_wait_for_completion(timeout_ms);
        if (ret != STATUS_SUCCESS) {
            return ret;
        }
    }
    
    // Send STOP condition
    REG_I2C_0_CMD = 0x20;  // STOP
    
    return STATUS_SUCCESS;
}

static status_t i2c_master_read_impl(i2c_port_t port, uint32_t slave_addr, 
                                   uint8_t* data, size_t length, uint32_t timeout_ms)
{
    CHECK_NULL(data);
    
    if (port != 0 || length == 0) {
        return STATUS_INVALID_ARG;
    }
    
    // Validate 7-bit address
    if (slave_addr > 0x7F) {
        return STATUS_INVALID_ARG;
    }
    
    // Send START condition + slave address + read bit (1)
    REG_I2C_0_TXR = (slave_addr << 1) | 0x01;  // Read bit = 1
    REG_I2C_0_CMD = 0x90;  // START + WRITE (address phase)
    
    // Wait for address phase completion
    status_t ret = i2c_wait_for_completion(timeout_ms);
    if (ret != STATUS_SUCCESS) {
        return ret;
    }
    
    // Read data bytes
    for (size_t i = 0; i < length; i++) {
        if (i == length - 1) {
            // Last byte - send NACK and STOP
            REG_I2C_0_CMD = 0x60;  // READ + NACK + STOP
        } else {
            // Not last byte - send ACK and continue
            REG_I2C_0_CMD = 0x40;  // READ + ACK
        }
        
        ret = i2c_wait_for_completion(timeout_ms);
        if (ret != STATUS_SUCCESS) {
            return ret;
        }
        
        data[i] = REG_I2C_0_RXR;
    }
    
    return STATUS_SUCCESS;
}

static status_t i2c_master_write_read_impl(i2c_port_t port, uint32_t slave_addr,
                                        const uint8_t* write_data, size_t write_length,
                                        uint8_t* read_data, size_t read_length, 
                                        uint32_t timeout_ms)
{
    // First perform write operation
    status_t ret = i2c_master_write_impl(port, slave_addr, write_data, write_length, timeout_ms);
    if (ret != STATUS_SUCCESS) {
        return ret;
    }
    
    // Then perform read operation (repeated start)
    return i2c_master_read_impl(port, slave_addr, read_data, read_length, timeout_ms);
}

static status_t i2c_is_bus_busy_impl(i2c_port_t port, bool_t* busy)
{
    CHECK_NULL(busy);
    
    if (port != 0) {
        return STATUS_INVALID_ARG;
    }
    
    // Check if I2C bus is busy (simplified check)
    // In real implementation, check actual bus lines or controller status
    *busy = FALSE;  // Assume not busy for now
    
    return STATUS_SUCCESS;
}