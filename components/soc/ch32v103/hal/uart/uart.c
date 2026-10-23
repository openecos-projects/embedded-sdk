/* Minimal CH32V103 polling USART1 backend, no vendor library dependency.
 * Register layout checked against openwch/ch32v103 EVT ch32v10x.h.
 * Console: PA9 TX / PA10 RX, HSI 8 MHz, APB2 prescaler 1, 115200 8N1.
 * Experimental: compile/link validated only; requires hardware validation.
 */
#include "ecos/hal/uart.h"

#include <limits.h>
#include <stdint.h>

#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define RCC_CTLR       REG32(0x40021000u)
#define RCC_CFGR0      REG32(0x40021004u)
#define RCC_APB2PCENR  REG32(0x40021018u)
#define GPIOA_CFGHR    REG32(0x40010804u)
#define USART1_STATR   REG32(0x40013800u)
#define USART1_DATAR   REG32(0x40013804u)
#define USART1_BRR     REG32(0x40013808u)
#define USART1_CTLR1   REG32(0x4001380cu)
#define USART1_CTLR2   REG32(0x40013810u)
#define USART1_CTLR3   REG32(0x40013814u)
#define CLOCK_TIMEOUT  1000000u

static int valid_buffer(hal_uart_port_t port, const void *data, size_t size)
{
    return port == HAL_UART_PORT_0 && (data != NULL || size == 0u) && size <= INT_MAX;
}

ecos_err_t hal_uart_init(hal_uart_port_t port, const hal_uart_config_t *config)
{
    uint32_t timeout;

    if (port != HAL_UART_PORT_0 || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (config->baud_rate != 115200u || config->data_bits != 8u ||
        config->stop_bits != 1u || config->parity != HAL_UART_PARITY_NONE)
        return ECOS_ERR_UNSUPPORTED;

    /* Select the internal oscillator; no board crystal/PLL assumptions. */
    RCC_CTLR |= 1u;
    for (timeout = CLOCK_TIMEOUT; (RCC_CTLR & (1u << 1)) == 0u; --timeout) {
        if (timeout == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    RCC_CFGR0 &= ~3u;
    for (timeout = CLOCK_TIMEOUT; (RCC_CFGR0 & (3u << 2)) != 0u; --timeout) {
        if (timeout == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    /* AHB/APB1/APB2 divide by one. USART1 uses APB2. */
    RCC_CFGR0 &= ~((15u << 4) | (7u << 8) | (7u << 11));
    RCC_APB2PCENR |= (1u << 2) | (1u << 14); /* IOPAEN, USART1EN */
    /* PA9 alternate-function push-pull 10 MHz; PA10 floating input. */
    GPIOA_CFGHR = (GPIOA_CFGHR & ~((15u << 4) | (15u << 8))) |
                  (9u << 4) | (4u << 8);
    USART1_CTLR1 = 0u;
    USART1_CTLR2 = 0u;
    USART1_CTLR3 = 0u;
    USART1_BRR = (8000000u + config->baud_rate / 2u) / config->baud_rate;
    USART1_CTLR1 = (1u << 13) | (1u << 3) | (1u << 2); /* UE, TE, RE */
    return ECOS_OK;
}

int hal_uart_write(hal_uart_port_t port, const uint8_t *data, size_t size)
{
    if (!valid_buffer(port, data, size))
        return ECOS_ERR_INVALID_ARGUMENT;
    for (size_t index = 0u; index < size; ++index) {
        while ((USART1_STATR & (1u << 7)) == 0u)
            ;
        USART1_DATAR = data[index];
    }
    return (int)size;
}

int hal_uart_read(hal_uart_port_t port, uint8_t *data, size_t size)
{
    if (!valid_buffer(port, data, size))
        return ECOS_ERR_INVALID_ARGUMENT;
    for (size_t index = 0u; index < size; ++index) {
        while ((USART1_STATR & (1u << 5)) == 0u)
            ;
        data[index] = (uint8_t)USART1_DATAR;
    }
    return (int)size;
}

int hal_uart_try_read(hal_uart_port_t port, uint8_t *data)
{
    if (!valid_buffer(port, data, 1u))
        return ECOS_ERR_INVALID_ARGUMENT;
    if ((USART1_STATR & (1u << 5)) == 0u)
        return 0;
    *data = (uint8_t)USART1_DATAR;
    return 1;
}
