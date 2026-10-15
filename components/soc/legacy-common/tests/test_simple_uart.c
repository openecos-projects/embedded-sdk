#include "ecos/hal/uart.h"

#include <assert.h>
#include <stdint.h>

volatile uint32_t test_uart0_clkdiv;
volatile uint32_t test_uart0_data;

int main(void)
{
    const hal_uart_config_t config = { 115200u, 8u, 1u, HAL_UART_PARITY_NONE };
    uint8_t byte = 0x5au;

    assert(hal_uart_init(HAL_UART_PORT_0, &config) == ECOS_OK);
    assert(test_uart0_clkdiv != 0u);

    test_uart0_data = UINT32_MAX;
    assert(hal_uart_try_read(HAL_UART_PORT_0, &byte) == 0);
    assert(byte == 0x5au);

    test_uart0_data = 0xffu;
    assert(hal_uart_try_read(HAL_UART_PORT_0, &byte) == 1);
    assert(byte == 0xffu);

    test_uart0_data = 'A';
    assert(hal_uart_read(HAL_UART_PORT_0, &byte, 1u) == 1);
    assert(byte == 'A');
    assert(hal_uart_try_read(HAL_UART_PORT_1, &byte) == ECOS_ERR_INVALID_ARGUMENT);
    return 0;
}
