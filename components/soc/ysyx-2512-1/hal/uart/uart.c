#include "ecos/hal/uart.h"
#include "ysyx_2512_1_soc.h"

#include <limits.h>

#define UART0_BAUD_RATE       115200u
#define UART0_DIVISOR         13u
#define UART_LCR_DLAB         0x80u
#define UART_LCR_8N1          0x03u
#define UART_LSR_DATA_READY   0x01u
#define UART_LSR_THR_EMPTY    0x20u

/* UART1 (hp block): TX = GPIO0[25], RX = GPIO0[26], alternate function 0. */
#define UART1_GPIO_MASK       ((uint32_t)0x3u << 25)
#define UART1_LCR_8N1         0x1Fu
#define UART1_FCR_RESET       0x0Fu
#define UART1_FCR_RUN         0x0Cu
#define UART1_LSR_RX_EMPTY    0x080u
#define UART1_LSR_TX_FULL     0x100u

static int uart_port_is_valid(hal_uart_port_t port)
{
    return port == HAL_UART_PORT_0 || port == HAL_UART_PORT_1;
}

static int uart0_config_is_supported(const hal_uart_config_t *config)
{
    return config->baud_rate == UART0_BAUD_RATE &&
           config->data_bits == 8u &&
           config->stop_bits == 1u &&
           config->parity == HAL_UART_PARITY_NONE;
}

static uint32_t uart1_clock_hz(void)
{
#ifdef CONFIG_CPU_FREQ_MHZ
    return (uint32_t)CONFIG_CPU_FREQ_MHZ * 1000000u;
#else
    return 50000000u;
#endif
}

/* The rv32e runtime links without libgcc division helpers, so the baud
 * divisor is computed with shift-subtract long division. */
static uint32_t uart1_divide(uint32_t numerator, uint32_t denominator)
{
    uint32_t quotient = 0u;
    uint32_t remainder = 0u;
    int bit;

    for (bit = 31; bit >= 0; --bit) {
        remainder = (remainder << 1) | ((numerator >> bit) & 1u);
        if (remainder >= denominator) {
            remainder -= denominator;
            quotient |= (uint32_t)1u << bit;
        }
    }
    return quotient;
}

static ecos_err_t uart1_init(const hal_uart_config_t *config)
{
    uint32_t clock;
    uint32_t divisor;

    /* Only the 8N1 frame format is supported for now; the legacy hp_uart
     * LCR encodings for other formats are unverified on hardware. */
    if (config->data_bits != 8u || config->stop_bits != 1u ||
        config->parity != HAL_UART_PARITY_NONE)
        return ECOS_ERR_UNSUPPORTED;

    clock = uart1_clock_hz();
    if (config->baud_rate == 0u || config->baud_rate > clock)
        return ECOS_ERR_UNSUPPORTED;
    divisor = uart1_divide(clock, config->baud_rate) - 1u;

    REG_GPIO_0_IOFCFG |= UART1_GPIO_MASK;
    REG_GPIO_0_PINMUX &= ~UART1_GPIO_MASK;

    REG_UART_1_LCR = 0u;
    REG_UART_1_DIV = divisor;
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

ecos_err_t hal_uart_init(hal_uart_port_t port,
                         const hal_uart_config_t *config)
{
    if (!uart_port_is_valid(port) || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (port == HAL_UART_PORT_1)
        return uart1_init(config);
    if (!uart0_config_is_supported(config))
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
    if (port == HAL_UART_PORT_1)
        return uart1_write(data, size);

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
    if (port == HAL_UART_PORT_1)
        return uart1_read(data, size);

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
    if (port == HAL_UART_PORT_1)
        return uart1_try_read(data);
    if ((REG_UART_0_LS & UART_LSR_DATA_READY) == 0u)
        return 0;

    *data = REG_UART_0_RB;
    return 1;
}
