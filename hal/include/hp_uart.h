/**
 * @file hp_uart.h
 * @brief High Performance UART Hardware Abstraction Layer interface
 */

#ifndef HP_UART_H__
#define HP_UART_H__

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HP UART port number type
 */
typedef uint32_t hp_uart_port_t;

/**
 * @brief HP UART baud rate enumeration
 */
typedef enum {
    HP_UART_BAUDRATE_9600 = 9600,
    HP_UART_BAUDRATE_19200 = 19200, 
    HP_UART_BAUDRATE_38400 = 38400,
    HP_UART_BAUDRATE_57600 = 57600,
    HP_UART_BAUDRATE_115200 = 115200
} hp_uart_baudrate_t;

/**
 * @brief HP UART stop bits enumeration
 */
typedef enum {
    HP_UART_STOP_BITS_1 = 1,
    HP_UART_STOP_BITS_2 = 2
} hp_uart_stop_bits_t;

/**
 * @brief HP UART parity enumeration
 */
typedef enum {
    HP_UART_PARITY_NONE = 0,
    HP_UART_PARITY_ODD = 1,
    HP_UART_PARITY_EVEN = 2,
    HP_UART_PARITY_ZERO = 3,
    HP_UART_PARITY_ONE = 4
} hp_uart_parity_t;

/**
 * @brief HP UART data bits enumeration
 */
typedef enum {
    HP_UART_DATA_5 = 5,
    HP_UART_DATA_6 = 6,
    HP_UART_DATA_7 = 7,
    HP_UART_DATA_8 = 8
} hp_uart_data_bits_t;

/**
 * @brief HP UART configuration structure
 */
typedef struct {
    hp_uart_baudrate_t baudrate;        /**< Baud rate */
    hp_uart_stop_bits_t stop_bits;      /**< Stop bits */
    hp_uart_parity_t parity;            /**< Parity */
    hp_uart_data_bits_t data_bits;      /**< Data bits */
} hp_uart_config_t;

/**
 * @brief HP UART driver interface structure
 */
typedef struct {
    driver_base_t base;                 /**< Base driver structure */
    
    /**
     * @brief Initialize HP UART with baud rate
     * @param port HP UART port number
     * @param baudrate Baud rate
     * @return Status code
     */
    status_t (*init)(hp_uart_port_t port, hp_uart_baudrate_t baudrate);
    
    /**
     * @brief Configure HP UART with full configuration
     * @param port HP UART port number
     * @param config Pointer to HP UART configuration
     * @return Status code
     */
    status_t (*config)(hp_uart_port_t port, const hp_uart_config_t* config);
    
    /**
     * @brief Send single character through HP UART
     * @param port HP UART port number
     * @param c Character to send
     * @return Status code
     */
    status_t (*send_char)(hp_uart_port_t port, char c);
    
    /**
     * @brief Send string through HP UART
     * @param port HP UART port number
     * @param str String to send
     * @return Status code
     */
    status_t (*send_string)(hp_uart_port_t port, const char* str);
    
    /**
     * @brief Receive single character from HP UART
     * @param port HP UART port number
     * @param c Pointer to store received character
     * @param timeout_ms Timeout in milliseconds (0 for no timeout)
     * @return Status code
     */
    status_t (*recv_char)(hp_uart_port_t port, char* c, uint32_t timeout_ms);
    
    /**
     * @brief Receive string from HP UART
     * @param port HP UART port number
     * @param buffer Pointer to receive buffer
     * @param max_length Maximum buffer length
     * @param timeout_ms Timeout in milliseconds (0 for no timeout)
     * @return Status code
     */
    status_t (*recv_string)(hp_uart_port_t port, char* buffer, size_t max_length, uint32_t timeout_ms);
    
} hp_uart_driver_t;

/**
 * @brief Register an HP UART driver
 * @param driver Pointer to the HP UART driver to register
 * @return Status code
 */
status_t hp_uart_register_driver(const hp_uart_driver_t* driver);

/**
 * @brief Get the currently registered HP UART driver
 * @return Pointer to the active HP UART driver, or NULL if none registered
 */
hp_uart_driver_t* hp_uart_get_driver(void);

#ifdef __cplusplus
}
#endif

#endif /* HP_UART_H__ */