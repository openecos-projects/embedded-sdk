#include "ecos/driver/uart.h"

#include "ecos/hal/uart.h"

static uint8_t uart_initialized[ECOS_UART_PORT_COUNT];

static int uart_port_is_valid(ecos_uart_port_t port)
{
    return port >= ECOS_UART_PORT_0 && port < ECOS_UART_PORT_COUNT;
}

static int uart_map_hal_result(int result)
{
    if (result >= 0)
        return result;
    if (result == HAL_UART_ERROR_INVALID_ARGUMENT)
        return ECOS_UART_ERROR_INVALID_ARGUMENT;
    if (result == HAL_UART_ERROR_UNSUPPORTED)
        return ECOS_UART_ERROR_UNSUPPORTED;
    return ECOS_UART_ERROR_IO;
}

int ecos_uart_init(ecos_uart_port_t port, const ecos_uart_config_t *config)
{
    hal_uart_config_t hal_config;
    int result;

    if (!uart_port_is_valid(port) || config == NULL)
        return ECOS_UART_ERROR_INVALID_ARGUMENT;
    if (config->parity < ECOS_UART_PARITY_NONE ||
        config->parity > ECOS_UART_PARITY_EVEN ||
        config->data_bits < 5u || config->data_bits > 8u ||
        (config->stop_bits != 1u && config->stop_bits != 2u) ||
        config->baud_rate == 0u)
        return ECOS_UART_ERROR_INVALID_ARGUMENT;

    hal_config.baud_rate = config->baud_rate;
    hal_config.data_bits = config->data_bits;
    hal_config.stop_bits = config->stop_bits;
    hal_config.parity = (hal_uart_parity_t)config->parity;
    result = uart_map_hal_result(hal_uart_init((hal_uart_port_t)port, &hal_config));
    if (result == ECOS_UART_OK)
        uart_initialized[port] = 1u;
    return result;
}

int ecos_uart_write(ecos_uart_port_t port, const void *data, size_t size)
{
    if (!uart_port_is_valid(port) || (data == NULL && size != 0u))
        return ECOS_UART_ERROR_INVALID_ARGUMENT;
    if (uart_initialized[port] == 0u)
        return ECOS_UART_ERROR_NOT_INITIALIZED;
    return uart_map_hal_result(
        hal_uart_write((hal_uart_port_t)port, (const uint8_t *)data, size)
    );
}

int ecos_uart_read(ecos_uart_port_t port, void *data, size_t size)
{
    if (!uart_port_is_valid(port) || (data == NULL && size != 0u))
        return ECOS_UART_ERROR_INVALID_ARGUMENT;
    if (uart_initialized[port] == 0u)
        return ECOS_UART_ERROR_NOT_INITIALIZED;
    return uart_map_hal_result(
        hal_uart_read((hal_uart_port_t)port, (uint8_t *)data, size)
    );
}

int ecos_uart_try_read(ecos_uart_port_t port, uint8_t *data)
{
    if (!uart_port_is_valid(port) || data == NULL)
        return ECOS_UART_ERROR_INVALID_ARGUMENT;
    if (uart_initialized[port] == 0u)
        return ECOS_UART_ERROR_NOT_INITIALIZED;
    return uart_map_hal_result(hal_uart_try_read((hal_uart_port_t)port, data));
}
