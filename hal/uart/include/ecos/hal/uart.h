#ifndef ECOS_HAL_UART_H
#define ECOS_HAL_UART_H

#include "ecos/error.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_UART_PORT_0 = 0,
    HAL_UART_PORT_COUNT
} hal_uart_port_t;

typedef enum {
    HAL_UART_PARITY_NONE = 0,
    HAL_UART_PARITY_ODD,
    HAL_UART_PARITY_EVEN
} hal_uart_parity_t;

typedef struct {
    uint32_t baud_rate;
    uint8_t data_bits;
    uint8_t stop_bits;
    hal_uart_parity_t parity;
} hal_uart_config_t;

ecos_err_t hal_uart_init(hal_uart_port_t port,
                         const hal_uart_config_t *config);
int hal_uart_write(hal_uart_port_t port, const uint8_t *data, size_t size);
int hal_uart_read(hal_uart_port_t port, uint8_t *data, size_t size);
int hal_uart_try_read(hal_uart_port_t port, uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif
