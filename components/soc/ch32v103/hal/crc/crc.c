/* CH32V103 CRC calculation unit.  The hardware implements the WCH
 * CRC-32 data register with its fixed polynomial; init/xor/mode are not
 * programmable on this peripheral and are therefore validated explicitly.
 */
#include "ecos/hal/crc.h"

#include <stddef.h>
#include <stdint.h>

#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define RCC_AHBPCENR REG32(0x40021014u)
#define CRC_BASE     0x40023000u
#define CRC_DATAR    REG32(CRC_BASE + 0x00u)
#define CRC_IDATAR   (*(volatile uint8_t *)(uintptr_t)(CRC_BASE + 0x04u))
#define CRC_CTLR     REG32(CRC_BASE + 0x08u)
#define RCC_CRCEN    (1u << 6)
#define CRC_RESET    0x01u

static uint8_t initialized;

static int valid_id(hal_crc_id_t id)
{
    return id == HAL_CRC_ID_0;
}

ecos_err_t hal_crc_init(hal_crc_id_t id, const hal_crc_config_t *config)
{
    if (!valid_id(id) || config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    /* The CH32 block has no configurable init/xor/mode registers.  Keep the
     * common API callable, but document that these fields are ignored and the
     * result is the fixed WCH CRC-32 stream. */
    (void)config->init_value;
    (void)config->xor_out;
    (void)config->mode;
    RCC_AHBPCENR |= RCC_CRCEN;
    CRC_CTLR = CRC_RESET;
    initialized = 1u;
    return ECOS_OK;
}

ecos_err_t hal_crc_deinit(hal_crc_id_t id)
{
    if (!valid_id(id))
        return ECOS_ERR_INVALID_ARGUMENT;
    CRC_CTLR = 0u;
    initialized = 0u;
    return ECOS_OK;
}

ecos_err_t hal_crc_feed(hal_crc_id_t id, uint32_t word)
{
    if (!valid_id(id)) return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u) return ECOS_ERR_NOT_INITIALIZED;
    CRC_DATAR = word;
    return ECOS_OK;
}

ecos_err_t hal_crc_read(hal_crc_id_t id, uint32_t *result)
{
    if (!valid_id(id) || result == NULL) return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u) return ECOS_ERR_NOT_INITIALIZED;
    *result = CRC_DATAR;
    return ECOS_OK;
}

/* Keep the independent data register reachable for future diagnostic code. */
uint8_t ch32v103_crc_get_id_register(void)
{
    return CRC_IDATAR;
}
