/**
 * @file sys_uart.h
 * @brief System UART Hardware Abstraction Layer interface
 * 
 * This interface is designed for simple system UART functionality,
 * typically used for debug output and basic console I/O.
 */

#ifndef SYS_UART_H__
#define SYS_UART_H__

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System UART configuration structure
 */
typedef struct {
    uint32_t baud_rate;     /**< Baud rate */
} sys_uart_config_t;

/**
 * @brief System UART driver interface structure
 */
typedef struct {
    driver_base_t base;     /**< Base driver structure */
    
    /**
     * @brief Initialize system UART
     * @param config Pointer to UART configuration (can be NULL for default)
     * @return Status code
     */
    status_t (*init)(const sys_uart_config_t* config);
    
    /**
     * @brief Send a single character
     * @param c Character to send
     * @return Status code
     */
    status_t (*putchar)(char c);
    
    /**
     * @brief Send a null-terminated string
     * @param str String to send
     * @return Status code
     */
    status_t (*puts)(const char* str);
    
    /**
     * @brief Send raw data
     * @param data Pointer to data buffer
     * @param length Number of bytes to send
     * @return Status code
     */
    status_t (*write)(const uint8_t* data, size_t length);
    
} sys_uart_driver_t;

/**
 * @brief Register a system UART driver
 * @param driver Pointer to the system UART driver to register
 * @return Status code
 */
status_t sys_uart_register_driver(const sys_uart_driver_t* driver);

/**
 * @brief Get the currently registered system UART driver
 * @return Pointer to the active system UART driver, or NULL if none registered
 */
sys_uart_driver_t* sys_uart_get_driver(void);

#ifdef __cplusplus
}
#endif

#endif /* SYS_UART_H__ */