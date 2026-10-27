/* CH32V103 on-chip Flash programming backend.  Programming is half-word
 * based; callers must erase before changing a programmed 0 back to 1. */
#include "ecos/hal/flash.h"

#include <stddef.h>
#include <stdint.h>

#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define FLASH_BASE_REG 0x40022000u
#define FLASH_ACTLR    REG32(FLASH_BASE_REG + 0x00u)
#define FLASH_KEYR     REG32(FLASH_BASE_REG + 0x04u)
#define FLASH_STATR    REG32(FLASH_BASE_REG + 0x0Cu)
#define FLASH_CTLR     REG32(FLASH_BASE_REG + 0x10u)
#define FLASH_ADDR     REG32(FLASH_BASE_REG + 0x14u)
#define FLASH_BSY      0x01u
#define FLASH_PGERR    0x04u
#define FLASH_WRPRTERR 0x10u
#define FLASH_EOP      0x20u
#define FLASH_PG       0x01u
#define FLASH_PER      0x02u
#define FLASH_MER      0x04u
#define FLASH_STRT     0x40u
#define FLASH_LOCK     0x80u
#define FLASH_KEY1     0x45670123u
#define FLASH_KEY2     0xCDEF89ABu
#define FLASH_TIMEOUT  200000u

static uint32_t physical(uint32_t offset)
{
    return HAL_FLASH_BASE_PHYSICAL + offset;
}

static int valid_id(hal_flash_id_t id) { return id == HAL_FLASH_ID_0; }

static void clear_status(void)
{
    FLASH_STATR = FLASH_PGERR | FLASH_WRPRTERR | FLASH_EOP;
}

static ecos_err_t wait_ready(void)
{
    uint32_t timeout = FLASH_TIMEOUT;
    while ((FLASH_STATR & FLASH_BSY) != 0u) {
        if (timeout-- == 0u) return ECOS_ERR_TIMEOUT;
    }
    if ((FLASH_STATR & (FLASH_PGERR | FLASH_WRPRTERR)) != 0u)
        return ECOS_ERR_IO;
    return ECOS_OK;
}

static ecos_err_t validate_range(uint32_t offset, size_t size)
{
    if (offset > HAL_FLASH_SIZE_BYTES || size > HAL_FLASH_SIZE_BYTES - offset)
        return ECOS_ERR_INVALID_ARGUMENT;
    return ECOS_OK;
}

static ecos_err_t unlock(void)
{
    ecos_err_t result = wait_ready();
    if (result != ECOS_OK) return result;
    clear_status();
    FLASH_KEYR = FLASH_KEY1;
    FLASH_KEYR = FLASH_KEY2;
    return ECOS_OK;
}

static void lock_flash(void) { FLASH_CTLR |= FLASH_LOCK; }

ecos_err_t hal_flash_get_size(hal_flash_id_t id, uint32_t *size)
{
    if (!valid_id(id) || size == NULL) return ECOS_ERR_INVALID_ARGUMENT;
    *size = HAL_FLASH_SIZE_BYTES;
    return ECOS_OK;
}

ecos_err_t hal_flash_read(hal_flash_id_t id, uint32_t offset,
                          void *data, size_t size)
{
    if (!valid_id(id) || data == NULL || size == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (validate_range(offset, size) != ECOS_OK) return ECOS_ERR_INVALID_ARGUMENT;
    for (size_t index = 0u; index < size; ++index)
        ((uint8_t *)data)[index] = *(volatile const uint8_t *)(uintptr_t)physical(offset + index);
    return ECOS_OK;
}

ecos_err_t hal_flash_write(hal_flash_id_t id, uint32_t offset,
                           const void *data, size_t size)
{
    ecos_err_t result;
    size_t index;

    if (!valid_id(id) || data == NULL || size == 0u ||
        (offset & 1u) != 0u || (size & 1u) != 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = validate_range(offset, size);
    if (result != ECOS_OK) return result;
    result = unlock();
    if (result != ECOS_OK) return result;
    FLASH_CTLR |= FLASH_PG;
    for (index = 0u; index < size; index += 2u) {
        uint16_t word = (uint16_t)((const uint8_t *)data)[index] |
                        (uint16_t)((uint16_t)((const uint8_t *)data)[index + 1u] << 8);
        *(volatile uint16_t *)(uintptr_t)physical(offset + (uint32_t)index) = word;
        result = wait_ready();
        if (result != ECOS_OK) break;
    }
    FLASH_CTLR &= ~FLASH_PG;
    lock_flash();
    return result;
}

ecos_err_t hal_flash_erase_page(hal_flash_id_t id, uint32_t offset)
{
    ecos_err_t result;
    if (!valid_id(id) || offset >= HAL_FLASH_SIZE_BYTES ||
        (offset % HAL_FLASH_PAGE_SIZE) != 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = unlock();
    if (result != ECOS_OK) return result;
    FLASH_CTLR |= FLASH_PER;
    FLASH_ADDR = physical(offset);
    FLASH_CTLR |= FLASH_STRT;
    result = wait_ready();
    FLASH_CTLR &= ~FLASH_PER;
    lock_flash();
    return result;
}

ecos_err_t hal_flash_erase_all(hal_flash_id_t id)
{
    ecos_err_t result;
    if (!valid_id(id)) return ECOS_ERR_INVALID_ARGUMENT;
    result = unlock();
    if (result != ECOS_OK) return result;
    FLASH_CTLR |= FLASH_MER;
    FLASH_CTLR |= FLASH_STRT;
    result = wait_ready();
    FLASH_CTLR &= ~FLASH_MER;
    lock_flash();
    return result;
}
