#include "ecos/bsp/console.h"

#include "ecos/driver/uart.h"

#include <limits.h>

static const ecos_uart_port_t console_uart = ECOS_UART_PORT_0;
static uint8_t previous_was_carriage_return;

static int console_map_uart_result(int result)
{
    if (result >= 0)
        return result;
    if (result == ECOS_UART_ERROR_INVALID_ARGUMENT)
        return BSP_CONSOLE_ERROR_INVALID_ARGUMENT;
    if (result == ECOS_UART_ERROR_UNSUPPORTED)
        return BSP_CONSOLE_ERROR_UNSUPPORTED;
    if (result == ECOS_UART_ERROR_NOT_INITIALIZED)
        return BSP_CONSOLE_ERROR_NOT_INITIALIZED;
    return BSP_CONSOLE_ERROR_IO;
}

int bsp_console_init(void)
{
    const ecos_uart_config_t config = ECOS_UART_CONFIG_DEFAULT;
    int result = ecos_uart_init(console_uart, &config);

    if (result == ECOS_UART_OK)
        previous_was_carriage_return = 0u;
    return console_map_uart_result(result);
}

int bsp_console_write(const char *text, size_t size)
{
    size_t consumed;

    if ((text == NULL && size != 0u) || size > INT_MAX)
        return BSP_CONSOLE_ERROR_INVALID_ARGUMENT;

    for (consumed = 0u; consumed < size; ++consumed) {
        int result;

        if (text[consumed] == '\n' && previous_was_carriage_return == 0u) {
            const char carriage_return = '\r';
            result = ecos_uart_write(console_uart, &carriage_return, 1u);
            if (result < 0)
                return consumed == 0u ?
                       console_map_uart_result(result) : (int)consumed;
        }
        result = ecos_uart_write(console_uart, &text[consumed], 1u);
        if (result < 0)
            return consumed == 0u ?
                   console_map_uart_result(result) : (int)consumed;
        previous_was_carriage_return = text[consumed] == '\r';
    }
    return (int)consumed;
}

int bsp_console_read(void *data, size_t size)
{
    uint8_t *bytes = (uint8_t *)data;
    int result;
    int index;

    if ((data == NULL && size != 0u) || size > INT_MAX)
        return BSP_CONSOLE_ERROR_INVALID_ARGUMENT;

    result = ecos_uart_read(console_uart, data, size);
    if (result < 0)
        return console_map_uart_result(result);
    for (index = 0; index < result; ++index) {
        if (bytes[index] == '\r')
            bytes[index] = '\n';
    }
    return result;
}

int bsp_console_try_read(uint8_t *data)
{
    int result;

    if (data == NULL)
        return BSP_CONSOLE_ERROR_INVALID_ARGUMENT;

    result = ecos_uart_try_read(console_uart, data);
    if (result < 0)
        return console_map_uart_result(result);
    if (result == 1 && *data == '\r')
        *data = '\n';
    return result;
}
