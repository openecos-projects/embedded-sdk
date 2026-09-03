#include "ecos/bsp/console.h"
#include "ecos/driver/uart.h"
#include "ecos/log.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static char uart_output[256];
static size_t uart_output_size;
static ecos_err_t uart_init_result = ECOS_OK;
static int uart_write_budget = -1;

ecos_err_t ecos_uart_init(ecos_uart_port_t port,
                          const ecos_uart_config_t *config)
{
    assert(port == ECOS_UART_PORT_0);
    assert(config != NULL);
    return uart_init_result;
}

int ecos_uart_write(ecos_uart_port_t port, const void *data, size_t size)
{
    assert(port == ECOS_UART_PORT_0);
    if (uart_write_budget == 0)
        return ECOS_ERR_IO;
    if (uart_write_budget > 0)
        --uart_write_budget;
    assert(uart_output_size + size <= sizeof(uart_output));
    memcpy(uart_output + uart_output_size, data, size);
    uart_output_size += size;
    return (int)size;
}

int ecos_uart_read(ecos_uart_port_t port, void *data, size_t size)
{
    (void)port;
    (void)data;
    (void)size;
    return 0;
}

int ecos_uart_try_read(ecos_uart_port_t port, uint8_t *data)
{
    (void)port;
    (void)data;
    return 0;
}

int main(void)
{
    static const char mixed_newlines[] = "a\nb\r\n";
    int result;

    assert(ecos_log_set_writer(NULL, NULL) == ECOS_OK);
    uart_init_result = ECOS_ERR_UNSUPPORTED;
    assert(bsp_console_init() == ECOS_ERR_UNSUPPORTED);
    assert(ECOS_LOGI("console", "unavailable") == ECOS_ERR_NOT_INITIALIZED);

    uart_init_result = ECOS_OK;
    assert(bsp_console_init() == ECOS_OK);
    result = ECOS_LOGI("console", "ready");
    assert(result == (int)strlen("[I][console] ready\r\n"));
    assert(uart_output_size == strlen("[I][console] ready\r\n"));
    assert(memcmp(uart_output, "[I][console] ready\r\n", uart_output_size) == 0);

    uart_output_size = 0u;
    result = bsp_console_write(mixed_newlines, sizeof(mixed_newlines) - 1u);
    assert(result == (int)(sizeof(mixed_newlines) - 1u));
    assert(uart_output_size == strlen("a\r\nb\r\n"));
    assert(memcmp(uart_output, "a\r\nb\r\n", uart_output_size) == 0);

    uart_output_size = 0u;
    uart_write_budget = 1;
    assert(bsp_console_write("\n", 1u) == ECOS_ERR_IO);
    assert(uart_output_size == 1u);
    assert(uart_output[0] == '\r');
    uart_write_budget = -1;
    assert(bsp_console_write("\n", 1u) == 1);
    assert(uart_output_size == 2u);
    assert(memcmp(uart_output, "\r\n", 2u) == 0);
    return 0;
}
