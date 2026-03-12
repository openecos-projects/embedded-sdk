/**
 * @file hp_uart_driver.h
 * @brief StarrySky C2 HP UART driver header
 */

#ifndef HP_UART_DRIVER_H__
#define HP_UART_DRIVER_H__

#include "hp_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize StarrySky C2 HP UART driver
 * @return Status code
 */
status_t starrysky_c2_hp_uart_init(void);

#ifdef __cplusplus
}
#endif

#endif /* HP_UART_DRIVER_H__ */