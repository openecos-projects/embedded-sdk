/**
 * @file qspi.h
 * @brief QSPI (Quad SPI) Hardware Abstraction Layer interface
 */

#ifndef QSPI_H__
#define QSPI_H__

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief QSPI port number type
 */
typedef uint32_t qspi_port_t;

/**
 * @brief QSPI chip select enumeration
 */
typedef enum {
    QSPI_CS_0 = 0,    /**< Chip Select 0 */
    QSPI_CS_1 = 1,    /**< Chip Select 1 */
    QSPI_CS_2 = 2,    /**< Chip Select 2 */
    QSPI_CS_3 = 3     /**< Chip Select 3 */
} qspi_cs_t;

/**
 * @brief QSPI configuration structure
 */
typedef struct {
    uint32_t clkdiv;    /**< Clock divider value */
} qspi_config_t;

/**
 * @brief QSPI driver interface structure
 */
typedef struct {
    driver_base_t base;             /**< Base driver structure */
    
    /**
     * @brief Initialize QSPI with configuration
     * @param port QSPI port number
     * @param config Pointer to QSPI configuration
     * @return Status code
     */
    status_t (*init)(qspi_port_t port, const qspi_config_t* config);
    
    /**
     * @brief Write 8-bit data through QSPI
     * @param port QSPI port number
     * @param data 8-bit data to write
     * @return Status code
     */
    status_t (*write_8)(qspi_port_t port, uint8_t data);
    
    /**
     * @brief Write 16-bit data through QSPI
     * @param port QSPI port number
     * @param data 16-bit data to write
     * @return Status code
     */
    status_t (*write_16)(qspi_port_t port, uint16_t data);
    
    /**
     * @brief Write 32-bit data through QSPI
     * @param port QSPI port number
     * @param data 32-bit data to write
     * @return Status code
     */
    status_t (*write_32)(qspi_port_t port, uint32_t data);
    
    /**
     * @brief Write multiple 32-bit data through QSPI (2 words)
     * @param port QSPI port number
     * @param data Array of 2 32-bit words
     * @return Status code
     */
    status_t (*write_32x2)(qspi_port_t port, const uint32_t* data);
    
    /**
     * @brief Write multiple 32-bit data through QSPI (8 words)
     * @param port QSPI port number
     * @param data Array of 8 32-bit words
     * @return Status code
     */
    status_t (*write_32x8)(qspi_port_t port, const uint32_t* data);
    
    /**
     * @brief Write multiple 32-bit data through QSPI (16 words)
     * @param port QSPI port number
     * @param data Array of 16 32-bit words
     * @return Status code
     */
    status_t (*write_32x16)(qspi_port_t port, const uint32_t* data);
    
    /**
     * @brief Write multiple 32-bit data through QSPI (32 words)
     * @param port QSPI port number
     * @param data Array of 32 32-bit words
     * @return Status code
     */
    status_t (*write_32x32)(qspi_port_t port, const uint32_t* data);
    
    /**
     * @brief Write 8-bit data with chip select
     * @param port QSPI port number
     * @param data 8-bit data to write
     * @param cs Chip select
     * @return Status code
     */
    status_t (*write_8_cs)(qspi_port_t port, uint8_t data, qspi_cs_t cs);
    
    /**
     * @brief Write 32-bit data with chip select
     * @param port QSPI port number
     * @param data 32-bit data to write
     * @param cs Chip select
     * @return Status code
     */
    status_t (*write_32_cs)(qspi_port_t port, uint32_t data, qspi_cs_t cs);
    
    /**
     * @brief Write multiple 32-bit data with chip select (2 words)
     * @param port QSPI port number
     * @param data Array of 2 32-bit words
     * @param cs Chip select
     * @return Status code
     */
    status_t (*write_32x2_cs)(qspi_port_t port, const uint32_t* data, qspi_cs_t cs);
    
    /**
     * @brief Write multiple 32-bit data with chip select (8 words)
     * @param port QSPI port number
     * @param data Array of 8 32-bit words
     * @param cs Chip select
     * @return Status code
     */
    status_t (*write_32x8_cs)(qspi_port_t port, const uint32_t* data, qspi_cs_t cs);
    
    /**
     * @brief Write multiple 32-bit data with chip select (16 words)
     * @param port QSPI port number
     * @param data Array of 16 32-bit words
     * @param cs Chip select
     * @return Status code
     */
    status_t (*write_32x16_cs)(qspi_port_t port, const uint32_t* data, qspi_cs_t cs);
    
    /**
     * @brief Write multiple 32-bit data with chip select (32 words)
     * @param port QSPI port number
     * @param data Array of 32 32-bit words
     * @param cs Chip select
     * @return Status code
     */
    status_t (*write_32x32_cs)(qspi_port_t port, const uint32_t* data, qspi_cs_t cs);
    
} qspi_driver_t;

/**
 * @brief Register a QSPI driver
 * @param driver Pointer to the QSPI driver to register
 * @return Status code
 */
status_t qspi_register_driver(const qspi_driver_t* driver);

/**
 * @brief Get the currently registered QSPI driver
 * @return Pointer to the active QSPI driver, or NULL if none registered
 */
qspi_driver_t* qspi_get_driver(void);

#ifdef __cplusplus
}
#endif

#endif /* QSPI_H__ */