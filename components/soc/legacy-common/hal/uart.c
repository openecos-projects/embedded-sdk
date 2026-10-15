#include "ecos/hal/uart.h"
#include "legacy_soc.h"

#include <limits.h>

#define UART_LCR_DLAB 0x80u
#define UART_LCR_8N1 0x03u
#define UART_LSR_DATA_READY 0x01u
#define UART_LSR_THR_EMPTY 0x20u
#define UART1_LCR_8N1 0x1Fu
#define UART1_FCR_RESET 0x0Fu
#define UART1_FCR_RUN 0x0Cu
#define UART1_LSR_RX_EMPTY 0x080u
#define UART1_LSR_TX_FULL 0x100u

static uint32_t divide_u32(uint32_t numerator, uint32_t denominator)
{
    uint32_t quotient = 0u;
    uint32_t remainder = 0u;
    int bit;

    if (denominator == 0u)
        return 0u;
    for (bit = 31; bit >= 0; --bit) {
        remainder = (remainder << 1) | ((numerator >> bit) & 1u);
        if (remainder >= denominator) {
            remainder -= denominator;
            quotient |= (uint32_t)1u << bit;
        }
    }
    return quotient;
}

static int port_is_valid(hal_uart_port_t port)
{
#if LEGACY_HAS_UART1
    return port == HAL_UART_PORT_0 || port == HAL_UART_PORT_1;
#else
    return port == HAL_UART_PORT_0;
#endif
}

static int config_is_8n1(const hal_uart_config_t *config)
{
    return config != NULL && config->data_bits == 8u &&
           config->stop_bits == 1u && config->parity == HAL_UART_PARITY_NONE &&
           config->baud_rate != 0u;
}

static uint32_t cpu_clock_hz(void)
{
#ifdef CONFIG_CPU_FREQ_MHZ
    return (uint32_t)CONFIG_CPU_FREQ_MHZ * 1000000u;
#else
    return 50000000u;
#endif
}

#if LEGACY_HAS_UART1
static void uart1_init_pins(void)
{
    LEGACY_UART1_INIT_PINS();
}

static ecos_err_t uart1_init(const hal_uart_config_t *config)
{
    uint32_t divisor;

    if (!config_is_8n1(config))
        return ECOS_ERR_UNSUPPORTED;
    divisor = divide_u32(cpu_clock_hz(), config->baud_rate);
    if (divisor == 0u)
        return ECOS_ERR_UNSUPPORTED;
    uart1_init_pins();
    REG_UART_1_LCR = 0u;
    REG_UART_1_DIV = divisor - 1u;
    REG_UART_1_FCR = UART1_FCR_RESET;
    REG_UART_1_FCR = UART1_FCR_RUN;
    REG_UART_1_LCR = UART1_LCR_8N1;
    return ECOS_OK;
}

static int uart1_write(const uint8_t *data, size_t size)
{
    size_t index;
    for (index = 0u; index < size; ++index) {
        while ((REG_UART_1_LSR & UART1_LSR_TX_FULL) != 0u)
            ;
        REG_UART_1_TRX = data[index];
    }
    return (int)size;
}

static int uart1_read(uint8_t *data, size_t size)
{
    size_t index;
    for (index = 0u; index < size; ++index) {
        while ((REG_UART_1_LSR & UART1_LSR_RX_EMPTY) != 0u)
            ;
        data[index] = REG_UART_1_TRX;
    }
    return (int)size;
}

static int uart1_try_read(uint8_t *data)
{
    if ((REG_UART_1_LSR & UART1_LSR_RX_EMPTY) != 0u)
        return 0;
    *data = REG_UART_1_TRX;
    return 1;
}
#endif

ecos_err_t hal_uart_init(hal_uart_port_t port, const hal_uart_config_t *config)
{
    if (!port_is_valid(port) || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
#if LEGACY_HAS_UART1
    if (port == HAL_UART_PORT_1)
        return uart1_init(config);
#endif
    if (!config_is_8n1(config))
        return ECOS_ERR_UNSUPPORTED;
#if LEGACY_UART0_SIMPLE
    REG_UART_0_CLKDIV = divide_u32(cpu_clock_hz(), config->baud_rate) - 1u;
#else
    REG_UART_0_LC = UART_LCR_DLAB;
    REG_UART_0_TH = (uint8_t)divide_u32(cpu_clock_hz(), config->baud_rate * 16u);
    REG_UART_0_IE = 0u;
    REG_UART_0_LC = UART_LCR_8N1;
    REG_UART_0_IE = 0u;
#endif
    return ECOS_OK;
}

int hal_uart_write(hal_uart_port_t port, const uint8_t *data, size_t size)
{
    size_t index;
    if (!port_is_valid(port) || (data == NULL && size != 0u) || size > INT_MAX)
        return ECOS_ERR_INVALID_ARGUMENT;
#if LEGACY_HAS_UART1
    if (port == HAL_UART_PORT_1)
        return uart1_write(data, size);
#endif
#if LEGACY_UART0_SIMPLE
    for (index = 0u; index < size; ++index)
        REG_UART_0_DATA = data[index];
#else
    for (index = 0u; index < size; ++index) {
        while ((REG_UART_0_LS & UART_LSR_THR_EMPTY) == 0u)
            ;
        REG_UART_0_TH = data[index];
    }
#endif
    return (int)size;
}

int hal_uart_read(hal_uart_port_t port, uint8_t *data, size_t size)
{
    size_t index;
    if (!port_is_valid(port) || (data == NULL && size != 0u) || size > INT_MAX)
        return ECOS_ERR_INVALID_ARGUMENT;
#if LEGACY_HAS_UART1
    if (port == HAL_UART_PORT_1)
        return uart1_read(data, size);
#endif
#if LEGACY_UART0_SIMPLE
    for (index = 0u; index < size; ++index) {
        uint32_t sample;

        do {
            sample = REG_UART_0_DATA;
        } while (sample == UINT32_MAX);
        data[index] = (uint8_t)sample;
    }
#else
    for (index = 0u; index < size; ++index) {
        while ((REG_UART_0_LS & UART_LSR_DATA_READY) == 0u)
            ;
        data[index] = REG_UART_0_RB;
    }
#endif
    return (int)size;
}

int hal_uart_try_read(hal_uart_port_t port, uint8_t *data)
{
    if (!port_is_valid(port) || data == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
#if LEGACY_HAS_UART1
    if (port == HAL_UART_PORT_1)
        return uart1_try_read(data);
#endif
#if LEGACY_UART0_SIMPLE
    uint32_t sample = REG_UART_0_DATA;

    if (sample == UINT32_MAX)
        return 0;
    *data = (uint8_t)sample;
    return 1;
#else
    if ((REG_UART_0_LS & UART_LSR_DATA_READY) == 0u)
        return 0;
    *data = REG_UART_0_RB;
    return 1;
#endif
}
