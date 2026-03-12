/**
 * @file i2c.h
 * @brief I2C Hardware Abstraction Layer interface
 */

#ifndef I2C_H__
#define I2C_H__

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C port number type
 */
typedef uint32_t i2c_port_t;

/**
 * @brief I2C address width enumeration
 */
typedef enum {
    I2C_ADDR_7BIT = 0,      /**< 7-bit address */
    I2C_ADDR_10BIT = 1      /**< 10-bit address */
} i2c_addr_width_t;

/**
 * @brief I2C clock speed enumeration
 */
typedef enum {
    I2C_SPEED_STANDARD = 100000,    /**< Standard mode: 100 kHz */
    I2C_SPEED_FAST = 400000,        /**< Fast mode: 400 kHz */
    I2C_SPEED_FAST_PLUS = 1000000   /**< Fast plus mode: 1 MHz */
} i2c_speed_t;

/**
 * @brief I2C configuration structure
 */
typedef struct {
    i2c_speed_t speed;              /**< I2C clock speed */
    i2c_addr_width_t addr_width;    /**< Address width */
    uint32_t slave_address;         /**< Slave address (for slave mode) */
} i2c_config_t;

/**
 * @brief I2C transfer direction
 */
typedef enum {
    I2C_WRITE = 0,                  /**< Write transfer */
    I2C_READ = 1                    /**< Read transfer */
} i2c_transfer_direction_t;

/**
 * @brief I2C driver interface structure
 */
typedef struct {
    driver_base_t base;             /**< Base driver structure */
    
    /**
     * @brief Initialize I2C with configuration
     * @param port I2C port number
     * @param config Pointer to I2C configuration
     * @return Status code
     */
    status_t (*init)(i2c_port_t port, const i2c_config_t* config);
    
    /**
     * @brief Deinitialize I2C
     * @param port I2C port number
     * @return Status code
     */
    status_t (*deinit)(i2c_port_t port);
    
    /**
     * @brief Master write data to I2C slave
     * @param port I2C port number
     * @param slave_addr Slave address
     * @param data Pointer to data buffer
     * @param length Number of bytes to write
     * @param timeout_ms Timeout in milliseconds (0 for no timeout)
     * @return Status code
     */
    status_t (*master_write)(i2c_port_t port, uint32_t slave_addr, 
                            const uint8_t* data, size_t length, uint32_t timeout_ms);
    
    /**
     * @brief Master read data from I2C slave
     * @param port I2C port number
     * @param slave_addr Slave address
     * @param data Pointer to receive buffer
     * @param length Number of bytes to read
     * @param timeout_ms Timeout in milliseconds (0 for no timeout)
     * @return Status code
     */
    status_t (*master_read)(i2c_port_t port, uint32_t slave_addr, 
                           uint8_t* data, size_t length, uint32_t timeout_ms);
    
    /**
     * @brief Master write then read (combined transfer)
     * @param port I2C port number
     * @param slave_addr Slave address
     * @param write_data Pointer to write data buffer
     * @param write_length Number of bytes to write
     * @param read_data Pointer to receive buffer
     * @param read_length Number of bytes to read
     * @param timeout_ms Timeout in milliseconds (0 for no timeout)
     * @return Status code
     */
    status_t (*master_write_read)(i2c_port_t port, uint32_t slave_addr,
                                 const uint8_t* write_data, size_t write_length,
                                 uint8_t* read_data, size_t read_length, 
                                 uint32_t timeout_ms);
    
    /**
     * @brief Check if I2C bus is busy
     * @param port I2C port number
     * @param busy Pointer to store busy status
     * @return Status code
     */
    status_t (*is_bus_busy)(i2c_port_t port, bool_t* busy);
    
} i2c_driver_t;

/**
 * @brief Register an I2C driver
 * @param driver Pointer to the I2C driver to register
 * @return Status code
 */
status_t i2c_register_driver(const i2c_driver_t* driver);

/**
 * @brief Get the currently registered I2C driver
 * @return Pointer to the active I2C driver, or NULL if none registered
 */
i2c_driver_t* i2c_get_driver(void);

#ifdef __cplusplus
}
#endif

#endif /* I2C_H__ */