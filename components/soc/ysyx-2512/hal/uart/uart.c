#include "ecos/hal/uart.h"
#include "ysyx_2512_soc.h"

#include <limits.h>

#define UART0_BAUD_RATE       115200u
#define UART0_DIVISOR         13u
#define UART_LCR_DLAB         0x80u
#define UART_LCR_8N1          0x03u
#define UART_LSR_DATA_READY   0x01u
#define UART_LSR_THR_EMPTY    0x20u

static int uart_port_is_valid(hal_uart_port_t port)
{
    return port == HAL_UART_PORT_0;
}

static int uart_config_is_supported(const hal_uart_config_t *config)
{
    return config != NULL &&
           config->baud_rate == UART0_BAUD_RATE &&
           config->data_bits == 8u &&
           config->stop_bits == 1u &&
           config->parity == HAL_UART_PARITY_NONE;
}

ecos_err_t hal_uart_init(hal_uart_port_t port,
                         const hal_uart_config_t *config)
{
    if (!uart_port_is_valid(port) || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (!uart_config_is_supported(config))
        return ECOS_ERR_UNSUPPORTED;

    REG_UART_0_LC = UART_LCR_DLAB;
    REG_UART_0_TH = UART0_DIVISOR;
    REG_UART_0_IE = 0u;
    REG_UART_0_LC = UART_LCR_8N1;
    REG_UART_0_IE = 0u;
    return ECOS_OK;
}

int hal_uart_write(hal_uart_port_t port, const uint8_t *data, size_t size)
{
    size_t index;

    if (!uart_port_is_valid(port) || (data == NULL && size != 0u) || size > INT_MAX)
        return ECOS_ERR_INVALID_ARGUMENT;

    for (index = 0u; index < size; ++index) {
        while ((REG_UART_0_LS & UART_LSR_THR_EMPTY) == 0u)
            ;
        REG_UART_0_TH = data[index];
    }
    return (int)size;
}

int hal_uart_read(hal_uart_port_t port, uint8_t *data, size_t size)
{
    size_t index;

    if (!uart_port_is_valid(port) || (data == NULL && size != 0u) || size > INT_MAX)
        return ECOS_ERR_INVALID_ARGUMENT;

    for (index = 0u; index < size; ++index) {
        while ((REG_UART_0_LS & UART_LSR_DATA_READY) == 0u)
            ;
        data[index] = REG_UART_0_RB;
    }
    return (int)size;
}

int hal_uart_try_read(hal_uart_port_t port, uint8_t *data)
{
    if (!uart_port_is_valid(port) || data == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if ((REG_UART_0_LS & UART_LSR_DATA_READY) == 0u)
        return 0;

    *data = REG_UART_0_RB;
    return 1;
}
