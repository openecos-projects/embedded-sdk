/* CH32V103 SPI1 polling master.
 * SCK=PA5, MISO=PA6, MOSI=PA7, software CS is owned by driver-gpio.
 */
#include "ecos/hal/spi.h"

#include <stddef.h>
#include <stdint.h>

#define REG16(address) (*(volatile uint16_t *)(uintptr_t)(address))
#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))

#define RCC_APB2PCENR REG32(0x40021018u)
#define AFIO_PCFR1    REG32(0x40010004u)
#define GPIOA_CFGLR   REG32(0x40010800u)
#define SPI1_BASE     0x40013000u
#define SPI1_CTLR1    REG16(SPI1_BASE + 0x00u)
#define SPI1_STATR    REG16(SPI1_BASE + 0x08u)
#define SPI1_DATAR    REG16(SPI1_BASE + 0x0Cu)

#define SPI_CPHA      0x0001u
#define SPI_CPOL      0x0002u
#define SPI_MSTR      0x0004u
#define SPI_SPE       0x0040u
#define SPI_LSBFIRST  0x0080u
#define SPI_SSI       0x0100u
#define SPI_SSM       0x0200u
#define SPI_RXNE      0x0001u
#define SPI_TXE       0x0002u
#define SPI_OVR       0x0040u
#define SPI_BSY       0x0080u
#define SPI_TIMEOUT   200000u

static uint8_t initialized;

static int valid_mode(hal_spi_mode_t mode)
{
    return mode >= HAL_SPI_MODE_0 && mode <= HAL_SPI_MODE_3;
}

static int valid_order(hal_spi_bit_order_t order)
{
    return order == HAL_SPI_BIT_ORDER_MSB_FIRST ||
           order == HAL_SPI_BIT_ORDER_LSB_FIRST;
}

static uint16_t baud_code(uint32_t clock_hz)
{
    static const uint32_t divisors[] = { 2u, 4u, 8u, 16u, 32u, 64u, 128u, 256u };
    uint16_t code;

    if (clock_hz == 0u)
        return 0xFFFFu;
    for (code = 0u; code < 8u; ++code) {
        if (8000000u / divisors[code] <= clock_hz)
            return (uint16_t)(code << 3);
    }
    return 7u << 3;
}

static int wait_status(uint16_t mask)
{
    uint32_t timeout = SPI_TIMEOUT;
    while ((SPI1_STATR & mask) == 0u) {
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    return ECOS_OK;
}

int hal_spi_get_instance_count(void)
{
    return 1;
}

ecos_err_t hal_spi_init(hal_spi_id_t spi, const hal_spi_config_t *config)
{
    uint16_t control;
    uint16_t baud;

    if (spi != 0u || config == NULL || !valid_mode(config->mode) ||
        !valid_order(config->bit_order))
        return ECOS_ERR_INVALID_ARGUMENT;
    baud = baud_code(config->clock_hz);
    if (baud == 0xFFFFu)
        return ECOS_ERR_INVALID_ARGUMENT;

    RCC_APB2PCENR |= (1u << 0) | (1u << 2) | (1u << 12); /* AFIO, GPIOA, SPI1 */
    AFIO_PCFR1 &= ~(1u << 0); /* SPI1 no remap: PA5/PA6/PA7 */
    /* PA5 SCK and PA7 MOSI alternate push-pull; PA6 MISO floating input. */
    GPIOA_CFGLR = (GPIOA_CFGLR & ~0xFFF00000u) | 0xB4B00000u;

    control = SPI_MSTR | SPI_SSM | SPI_SSI | baud;
    if (config->mode & 1u)
        control |= SPI_CPHA;
    if (config->mode & 2u)
        control |= SPI_CPOL;
    if (config->bit_order == HAL_SPI_BIT_ORDER_LSB_FIRST)
        control |= SPI_LSBFIRST;
    SPI1_CTLR1 = control;
    SPI1_CTLR1 |= SPI_SPE;
    initialized = 1u;
    return ECOS_OK;
}

ecos_err_t hal_spi_deinit(hal_spi_id_t spi)
{
    if (spi != 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    SPI1_CTLR1 &= (uint16_t)~SPI_SPE;
    initialized = 0u;
    return ECOS_OK;
}

ecos_err_t hal_spi_transfer_byte(hal_spi_id_t spi, uint8_t tx, uint8_t *rx)
{
    int result;
    uint32_t timeout;

    if (spi != 0u || rx == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    result = wait_status(SPI_TXE);
    if (result != ECOS_OK)
        return (ecos_err_t)result;
    SPI1_DATAR = tx;
    result = wait_status(SPI_RXNE);
    if (result != ECOS_OK)
        return (ecos_err_t)result;
    *rx = (uint8_t)SPI1_DATAR;
    timeout = SPI_TIMEOUT;
    while ((SPI1_STATR & SPI_BSY) != 0u) {
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    if ((SPI1_STATR & SPI_OVR) != 0u) {
        volatile uint16_t status = SPI1_STATR;
        volatile uint16_t data = SPI1_DATAR;
        (void)status;
        (void)data;
        return ECOS_ERR_IO;
    }
    return ECOS_OK;
}
