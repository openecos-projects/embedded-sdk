/* CH32V103 polling USART backend.
 * Port 0: USART1 PA9/PA10 (console), port 1: USART2 PA2/PA3,
 * port 2: USART3 PB10/PB11. All ports use the internal 8 MHz HSI and 8N1.
 */
#include "ecos/hal/uart.h"

#include <limits.h>
#include <stdint.h>

#define REG16(address) (*(volatile uint16_t *)(uintptr_t)(address))
#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define RCC_CTLR       REG32(0x40021000u)
#define RCC_CFGR0      REG32(0x40021004u)
#define RCC_APB2PCENR  REG32(0x40021018u)
#define RCC_APB1PCENR  REG32(0x4002101Cu)
#define AFIO_PCFR1     REG32(0x40010004u)
#define GPIOA_CFGLR    REG32(0x40010800u)
#define GPIOA_CFGHR    REG32(0x40010804u)
#define GPIOB_CFGHR    REG32(0x40010C04u)
#define USART1_BASE    0x40013800u
#define USART2_BASE    0x40004400u
#define USART3_BASE    0x40004800u
#define USART_STATR(base) REG16((base) + 0x00u)
#define USART_DATAR(base) REG16((base) + 0x04u)
#define USART_BRR(base)   REG16((base) + 0x08u)
#define USART_CTLR1(base) REG16((base) + 0x0Cu)
#define USART_CTLR2(base) REG16((base) + 0x10u)
#define USART_CTLR3(base) REG16((base) + 0x14u)
#define CLOCK_TIMEOUT 1000000u
#define HSI_HZ 8000000u

static int valid_port(hal_uart_port_t port)
{
    return port >= HAL_UART_PORT_0 && port < HAL_UART_PORT_COUNT;
}

static int valid_buffer(hal_uart_port_t port, const void *data, size_t size)
{
    return valid_port(port) && (data != NULL || size == 0u) && size <= INT_MAX;
}

static uint32_t usart_base(hal_uart_port_t port)
{
    return port == HAL_UART_PORT_0 ? USART1_BASE :
           (port == HAL_UART_PORT_1 ? USART2_BASE : USART3_BASE);
}

static ecos_err_t configure_hsi(void)
{
    uint32_t timeout;

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
    RCC_CFGR0 &= ~((15u << 4) | (7u << 8) | (7u << 11));
    return ECOS_OK;
}

static void configure_pins(hal_uart_port_t port)
{
    AFIO_PCFR1 &= ~((1u << 2) | (1u << 3) | (3u << 4));
    if (port == HAL_UART_PORT_0) {
        /* PA9 alternate-function push-pull 10 MHz; PA10 floating input. */
        GPIOA_CFGHR = (GPIOA_CFGHR & ~((15u << 4) | (15u << 8))) |
                      (9u << 4) | (4u << 8);
    } else if (port == HAL_UART_PORT_1) {
        /* PA2 alternate-function push-pull 10 MHz; PA3 floating input. */
        GPIOA_CFGLR = (GPIOA_CFGLR & ~((15u << 8) | (15u << 12))) |
                      (9u << 8) | (4u << 12);
    } else {
        /* PB10 alternate-function push-pull 10 MHz; PB11 floating input. */
        GPIOB_CFGHR = (GPIOB_CFGHR & ~((15u << 8) | (15u << 12))) |
                      (9u << 8) | (4u << 12);
    }
}

static void enable_clock(hal_uart_port_t port)
{
    RCC_APB2PCENR |= 1u | (1u << (2u + (port == HAL_UART_PORT_2 ? 1u : 0u)));
    if (port == HAL_UART_PORT_0)
        RCC_APB2PCENR |= 1u << 14;
    else
        RCC_APB1PCENR |= 1u << (17u + (uint32_t)(port - HAL_UART_PORT_1));
}

ecos_err_t hal_uart_init(hal_uart_port_t port, const hal_uart_config_t *config)
{
    uint32_t base;

    if (!valid_port(port) || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (config->baud_rate == 0u || config->baud_rate > HSI_HZ ||
        config->data_bits != 8u || config->stop_bits != 1u ||
        config->parity != HAL_UART_PARITY_NONE)
        return ECOS_ERR_UNSUPPORTED;

    if (configure_hsi() != ECOS_OK)
        return ECOS_ERR_TIMEOUT;
    enable_clock(port);
    configure_pins(port);
    base = usart_base(port);
    USART_CTLR1(base) = 0u;
    USART_CTLR2(base) = 0u;
    USART_CTLR3(base) = 0u;
    USART_BRR(base) = (HSI_HZ + config->baud_rate / 2u) / config->baud_rate;
    USART_CTLR1(base) = (1u << 13) | (1u << 3) | (1u << 2); /* UE, TE, RE */
    return ECOS_OK;
}

int hal_uart_write(hal_uart_port_t port, const uint8_t *data, size_t size)
{
    uint32_t base;
    if (!valid_buffer(port, data, size)) return ECOS_ERR_INVALID_ARGUMENT;
    base = usart_base(port);
    for (size_t index = 0u; index < size; ++index) {
        while ((USART_STATR(base) & (1u << 7)) == 0u) { }
        USART_DATAR(base) = data[index];
    }
    return (int)size;
}

int hal_uart_read(hal_uart_port_t port, uint8_t *data, size_t size)
{
    uint32_t base;
    if (!valid_buffer(port, data, size)) return ECOS_ERR_INVALID_ARGUMENT;
    base = usart_base(port);
    for (size_t index = 0u; index < size; ++index) {
        while ((USART_STATR(base) & (1u << 5)) == 0u) { }
        data[index] = (uint8_t)USART_DATAR(base);
    }
    return (int)size;
}

int hal_uart_try_read(hal_uart_port_t port, uint8_t *data)
{
    uint32_t base;
    if (!valid_buffer(port, data, 1u)) return ECOS_ERR_INVALID_ARGUMENT;
    base = usart_base(port);
    if ((USART_STATR(base) & (1u << 5)) == 0u) return 0;
    *data = (uint8_t)USART_DATAR(base);
    return 1;
}
