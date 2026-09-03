#ifndef ECOS_DRIVER_UART_H
#define ECOS_DRIVER_UART_H

#include "ecos/error.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECOS_UART_PORT_0 = 0,
    ECOS_UART_PORT_COUNT
} ecos_uart_port_t;

typedef enum {
    ECOS_UART_PARITY_NONE = 0,
    ECOS_UART_PARITY_ODD,
    ECOS_UART_PARITY_EVEN
} ecos_uart_parity_t;

typedef struct {
    uint32_t baud_rate;
    uint8_t data_bits;
    uint8_t stop_bits;
    ecos_uart_parity_t parity;
} ecos_uart_config_t;

#define ECOS_UART_CONFIG_DEFAULT \
    { 115200u, 8u, 1u, ECOS_UART_PARITY_NONE }

/* Initialize a UART instance. Returns ECOS_OK or a negative error code. */
ecos_err_t ecos_uart_init(ecos_uart_port_t port,
                          const ecos_uart_config_t *config);

/* Raw UART access: returns bytes transferred or a negative error code. */
int ecos_uart_write(ecos_uart_port_t port, const void *data, size_t size);
int ecos_uart_read(ecos_uart_port_t port, void *data, size_t size);

/* Returns 1 when a byte is read, 0 when no byte is ready, or a negative error. */
int ecos_uart_try_read(ecos_uart_port_t port, uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif
