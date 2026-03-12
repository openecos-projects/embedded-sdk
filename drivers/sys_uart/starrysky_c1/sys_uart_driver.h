/**
 * @file sys_uart_driver.h
 * @brief StarrySky C1 System UART driver header
 */

#ifndef SYS_UART_DRIVER_H__
#define SYS_UART_DRIVER_H__

#include "sys_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize StarrySky C1 System UART driver
 * @return Status code
 */
status_t starrysky_c1_sys_uart_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SYS_UART_DRIVER_H__ */