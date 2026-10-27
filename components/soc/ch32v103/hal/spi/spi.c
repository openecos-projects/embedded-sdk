/* CH32V103 polling SPI masters.
 * SPI1 (id 0): PA5/PA6/PA7; SPI2 (id 1): PB13/PB14/PB15.
 * Chip select remains a software GPIO owned by driver-spi.
 */
#include "ecos/hal/spi.h"

#include <stddef.h>
#include <stdint.h>

#define REG16(address) (*(volatile uint16_t *)(uintptr_t)(address))
#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define RCC_APB2PCENR REG32(0x40021018u)
#define RCC_APB1PCENR REG32(0x4002101Cu)
#define AFIO_PCFR1    REG32(0x40010004u)
#define GPIOA_CFGLR   REG32(0x40010800u)
#define GPIOB_CFGHR   REG32(0x40010C04u)
#define SPI1_BASE     0x40013000u
#define SPI2_BASE     0x40003800u
#define SPI_CTLR1(base) REG16((base) + 0x00u)
#define SPI_STATR(base) REG16((base) + 0x08u)
#define SPI_DATAR(base) REG16((base) + 0x0Cu)

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

static uint8_t initialized[2];

static int valid_id(hal_spi_id_t spi) { return spi < 2u; }
static int valid_mode(hal_spi_mode_t mode)
{
    return mode >= HAL_SPI_MODE_0 && mode <= HAL_SPI_MODE_3;
}
static int valid_order(hal_spi_bit_order_t order)
{
    return order == HAL_SPI_BIT_ORDER_MSB_FIRST ||
           order == HAL_SPI_BIT_ORDER_LSB_FIRST;
}
static uint32_t spi_base(hal_spi_id_t spi)
{
    return spi == 0u ? SPI1_BASE : SPI2_BASE;
}

static uint16_t baud_code(uint32_t clock_hz)
{
    static const uint32_t divisors[] = { 2u, 4u, 8u, 16u, 32u, 64u, 128u, 256u };
    uint16_t code;
    if (clock_hz == 0u) return 0xFFFFu;
    for (code = 0u; code < 8u; ++code)
        if (8000000u / divisors[code] <= clock_hz)
            return (uint16_t)(code << 3);
    return 7u << 3;
}

static void configure_pins(hal_spi_id_t spi)
{
    if (spi == 0u) {
        AFIO_PCFR1 &= ~(1u << 0);
        GPIOA_CFGLR = (GPIOA_CFGLR & ~0xFFF00000u) | 0xB4B00000u;
    } else {
        GPIOB_CFGHR = (GPIOB_CFGHR & ~0xFFF00000u) | 0xB4B00000u;
    }
}

static int wait_status(uint32_t base, uint16_t mask)
{
    uint32_t timeout = SPI_TIMEOUT;
    while ((SPI_STATR(base) & mask) == 0u) {
        if (timeout-- == 0u) return ECOS_ERR_TIMEOUT;
    }
    return ECOS_OK;
}

int hal_spi_get_instance_count(void) { return 2; }

ecos_err_t hal_spi_init(hal_spi_id_t spi, const hal_spi_config_t *config)
{
    uint16_t control;
    uint16_t baud;
    uint32_t base;

    if (!valid_id(spi) || config == NULL || !valid_mode(config->mode) ||
        !valid_order(config->bit_order))
        return ECOS_ERR_INVALID_ARGUMENT;
    baud = baud_code(config->clock_hz);
    if (baud == 0xFFFFu) return ECOS_ERR_INVALID_ARGUMENT;
    if (spi == 0u) {
        RCC_APB2PCENR |= (1u << 0) | (1u << 2) | (1u << 12);
    } else {
        RCC_APB2PCENR |= 1u | (1u << 3);
        RCC_APB1PCENR |= (1u << 14);
    }
    configure_pins(spi);
    base = spi_base(spi);
    control = SPI_MSTR | SPI_SSM | SPI_SSI | baud;
    if (config->mode & 1u) control |= SPI_CPHA;
    if (config->mode & 2u) control |= SPI_CPOL;
    if (config->bit_order == HAL_SPI_BIT_ORDER_LSB_FIRST) control |= SPI_LSBFIRST;
    SPI_CTLR1(base) = control | SPI_SPE;
    initialized[spi] = 1u;
    return ECOS_OK;
}

ecos_err_t hal_spi_deinit(hal_spi_id_t spi)
{
    if (!valid_id(spi)) return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized[spi] == 0u) return ECOS_ERR_NOT_INITIALIZED;
    SPI_CTLR1(spi_base(spi)) &= (uint16_t)~SPI_SPE;
    initialized[spi] = 0u;
    return ECOS_OK;
}

ecos_err_t hal_spi_transfer_byte(hal_spi_id_t spi, uint8_t tx, uint8_t *rx)
{
    int result;
    uint32_t base;
    uint32_t timeout;

    if (!valid_id(spi) || rx == NULL) return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized[spi] == 0u) return ECOS_ERR_NOT_INITIALIZED;
    base = spi_base(spi);
    result = wait_status(base, SPI_TXE);
    if (result != ECOS_OK) return (ecos_err_t)result;
    SPI_DATAR(base) = tx;
    result = wait_status(base, SPI_RXNE);
    if (result != ECOS_OK) return (ecos_err_t)result;
    *rx = (uint8_t)SPI_DATAR(base);
    timeout = SPI_TIMEOUT;
    while ((SPI_STATR(base) & SPI_BSY) != 0u)
        if (timeout-- == 0u) return ECOS_ERR_TIMEOUT;
    if ((SPI_STATR(base) & SPI_OVR) != 0u) {
        volatile uint16_t status = SPI_STATR(base);
        volatile uint16_t data = SPI_DATAR(base);
        (void)status; (void)data;
        return ECOS_ERR_IO;
    }
    return ECOS_OK;
}
