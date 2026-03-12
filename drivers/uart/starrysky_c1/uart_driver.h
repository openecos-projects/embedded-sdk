/**
 * @file uart_driver.h
 * @brief StarrySky C1 UART driver header
 */

#ifndef UART_DRIVER_H__
#define UART_DRIVER_H__

#include "hal_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize StarrySky C1 UART driver
 * @return HAL status code
 */
hal_status_t starrysky_c1_uart_init(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_DRIVER_H__ */