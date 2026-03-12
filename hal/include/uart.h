/**
 * @file hal_uart.h
 * @brief UART Hardware Abstraction Layer interface
 */

#ifndef HAL_UART_H__
#define HAL_UART_H__

#include <stdint.h>
#include "hal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UART port number type
 */
typedef uint32_t hal_uart_port_t;

/**
 * @brief UART data bits enumeration
 */
typedef enum {
    HAL_UART_DATA_5 = 5,    /**< 5 data bits */
    HAL_UART_DATA_6 = 6,    /**< 6 data bits */
    HAL_UART_DATA_7 = 7,    /**< 7 data bits */
    HAL_UART_DATA_8 = 8     /**< 8 data bits */
} hal_uart_data_bits_t;

/**
 * @brief UART stop bits enumeration
 */
typedef enum {
    HAL_UART_STOP_1 = 1,    /**< 1 stop bit */
    HAL_UART_STOP_2 = 2     /**< 2 stop bits */
} hal_uart_stop_bits_t;

/**
 * @brief UART parity enumeration
 */
typedef enum {
    HAL_UART_PARITY_NONE = 0,   /**< No parity */
    HAL_UART_PARITY_EVEN = 1,   /**< Even parity */
    HAL_UART_PARITY_ODD = 2     /**< Odd parity */
} hal_uart_parity_t;

/**
 * @brief UART configuration structure
 */
typedef struct {
    uint32_t baud_rate;             /**< Baud rate */
    hal_uart_data_bits_t data_bits; /**< Data bits */
    hal_uart_stop_bits_t stop_bits; /**< Stop bits */
    hal_uart_parity_t parity;       /**< Parity */
} hal_uart_config_t;

/**
 * @brief UART driver interface structure
 */
typedef struct {
    hal_driver_base_t base;         /**< Base driver structure */
    
    /**
     * @brief Initialize UART with configuration
     * @param port UART port number
     * @param config Pointer to UART configuration
     * @return HAL status code
     */
    hal_status_t (*init)(hal_uart_port_t port, const hal_uart_config_t* config);
    
    /**
     * @brief Deinitialize UART
     * @param port UART port number
     * @return HAL status code
     */
    hal_status_t (*deinit)(hal_uart_port_t port);
    
    /**
     * @brief Send data through UART
     * @param port UART port number
     * @param data Pointer to data buffer
     * @param length Number of bytes to send
     * @return HAL status code
     */
    hal_status_t (*send)(hal_uart_port_t port, const uint8_t* data, size_t length);
    
    /**
     * @brief Receive data from UART
     * @param port UART port number
     * @param data Pointer to receive buffer
     * @param length Number of bytes to receive
     * @param timeout_ms Timeout in milliseconds (0 for no timeout)
     * @return HAL status code
     */
    hal_status_t (*receive)(hal_uart_port_t port, uint8_t* data, size_t length, uint32_t timeout_ms);
    
    /**
     * @brief Get number of bytes available in receive buffer
     * @param port UART port number
     * @param available Pointer to store available bytes count
     * @return HAL status code
     */
    hal_status_t (*get_available)(hal_uart_port_t port, size_t* available);
    
} hal_uart_driver_t;

/**
 * @brief Register a UART driver
 * @param driver Pointer to the UART driver to register
 * @return HAL status code
 */
hal_status_t hal_uart_register_driver(const hal_uart_driver_t* driver);

/**
 * @brief Get the currently registered UART driver
 * @return Pointer to the active UART driver, or NULL if none registered
 */
hal_uart_driver_t* hal_uart_get_driver(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_UART_H__ */