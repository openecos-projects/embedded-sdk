#include "ecos/hal/backup.h"

#include <stddef.h>
#include <stdint.h>

#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define RCC_APB1PCENR REG32(0x4002101Cu)
#define RCC_BDCTLR    REG32(0x40021020u)
#define PWR_CTLR      REG32(0x40007000u)
#define BKP_BASE      0x40006C00u
#define RCC_BKPEN     (1u << 27)
#define RCC_PWREN     (1u << 28)
#define PWR_DBP       (1u << 8)
#define RCC_BDCRST    (1u << 16)

static int valid(hal_backup_id_t id, uint8_t index)
{
    return id == HAL_BACKUP_ID_0 && index < 42u;
}

static volatile uint16_t *register_address(uint8_t index)
{
    /* DR1..DR10 are at 0x04..0x28, DR11..DR42 at 0x40..0xBC. */
    uint32_t offset = index < 10u ? 0x04u + 4u * index : 0x40u + 4u * (index - 10u);
    return (volatile uint16_t *)(uintptr_t)(BKP_BASE + offset);
}

static void enable(void)
{
    RCC_APB1PCENR |= RCC_BKPEN | RCC_PWREN;
    PWR_CTLR |= PWR_DBP;
}

ecos_err_t hal_backup_read(hal_backup_id_t id, uint8_t index, uint16_t *value)
{
    if (!valid(id, index) || value == NULL) return ECOS_ERR_INVALID_ARGUMENT;
    enable();
    *value = *register_address(index);
    return ECOS_OK;
}

ecos_err_t hal_backup_write(hal_backup_id_t id, uint8_t index, uint16_t value)
{
    if (!valid(id, index)) return ECOS_ERR_INVALID_ARGUMENT;
    enable();
    *register_address(index) = value;
    return ECOS_OK;
}

ecos_err_t hal_backup_reset(hal_backup_id_t id)
{
    if (id != HAL_BACKUP_ID_0) return ECOS_ERR_INVALID_ARGUMENT;
    enable();
    RCC_BDCTLR |= RCC_BDCRST;
    RCC_BDCTLR &= ~RCC_BDCRST;
    return ECOS_OK;
}
