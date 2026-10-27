/* CH32V103 PWR low-power and reset flag backend. */
#include "ecos/hal/power.h"

#include <stddef.h>
#include <stdint.h>

#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define RCC_APB1PCENR REG32(0x4002101Cu)
#define RCC_RSTSCKR   REG32(0x40021024u)
#define PWR_CTLR      REG32(0x40007000u)
#define PWR_CSR       REG32(0x40007004u)
#define PFIC_SCTLR    REG32(0xE000E000u + 0xD10u)
#define RCC_PWREN     (1u << 28)
#define PWR_DBP       (1u << 8)
#define PWR_PVDE      (1u << 4)
#define PWR_PLS_MASK  (7u << 5)
#define PWR_WKUP      (1u << 8)
#define PWR_CWUF      (1u << 2)
#define PWR_PDDS      (1u << 1)
#define PWR_LPDS      (1u << 0)
#define PWR_FLAGS     0x07u
#define RCC_RMVF      0x01000000u

static int valid_id(hal_power_id_t id) { return id == HAL_POWER_ID_0; }

static void wfi(void)
{
    PFIC_SCTLR &= ~(1u << 3);
    __asm__ volatile("wfi");
}

static void wfe(void)
{
    PFIC_SCTLR |= (1u << 3);
    __asm__ volatile("wfi");
    PFIC_SCTLR &= ~(1u << 3);
}

ecos_err_t hal_power_set_backup_access(hal_power_id_t id, uint8_t enable)
{
    if (!valid_id(id)) return ECOS_ERR_INVALID_ARGUMENT;
    RCC_APB1PCENR |= RCC_PWREN;
    if (enable != 0u) PWR_CTLR |= PWR_DBP;
    else PWR_CTLR &= ~PWR_DBP;
    return ECOS_OK;
}

ecos_err_t hal_power_set_wakeup_pin(hal_power_id_t id, uint8_t enable)
{
    if (!valid_id(id)) return ECOS_ERR_INVALID_ARGUMENT;
    RCC_APB1PCENR |= RCC_PWREN;
    if (enable != 0u) PWR_CSR |= PWR_WKUP;
    else PWR_CSR &= ~PWR_WKUP;
    return ECOS_OK;
}

ecos_err_t hal_power_set_pvd(hal_power_id_t id, uint8_t enable, uint8_t level)
{
    if (!valid_id(id) || level > 7u) return ECOS_ERR_INVALID_ARGUMENT;
    RCC_APB1PCENR |= RCC_PWREN;
    PWR_CTLR = (PWR_CTLR & ~PWR_PLS_MASK) | ((uint32_t)level << 5);
    if (enable != 0u) PWR_CTLR |= PWR_PVDE;
    else PWR_CTLR &= ~PWR_PVDE;
    return ECOS_OK;
}

ecos_err_t hal_power_get_flags(hal_power_id_t id, uint32_t *flags)
{
    if (!valid_id(id) || flags == NULL) return ECOS_ERR_INVALID_ARGUMENT;
    *flags = PWR_CSR & PWR_FLAGS;
    return ECOS_OK;
}

ecos_err_t hal_power_clear_flags(hal_power_id_t id, uint32_t flags)
{
    if (!valid_id(id) || (flags & ~0x03u) != 0u) return ECOS_ERR_INVALID_ARGUMENT;
    PWR_CTLR |= (flags & 0x03u) << 2;
    RCC_RSTSCKR |= RCC_RMVF;
    return ECOS_OK;
}

ecos_err_t hal_power_enter(hal_power_id_t id, hal_power_mode_t mode)
{
    if (!valid_id(id) || mode > HAL_POWER_STANDBY)
        return ECOS_ERR_INVALID_ARGUMENT;
    RCC_APB1PCENR |= RCC_PWREN;
    if (mode == HAL_POWER_SLEEP_WFI) { wfi(); return ECOS_OK; }
    if (mode == HAL_POWER_SLEEP_WFE) { wfe(); return ECOS_OK; }
    if (mode == HAL_POWER_STANDBY) {
        PWR_CTLR |= PWR_CWUF | PWR_PDDS;
        PFIC_SCTLR |= 1u << 2;
        wfi();
        PFIC_SCTLR &= ~(1u << 2);
        return ECOS_OK;
    }
    PWR_CTLR = (PWR_CTLR & ~(PWR_PDDS | PWR_LPDS)) |
               (mode == HAL_POWER_STOP_WFI ? 0u : PWR_LPDS);
    PFIC_SCTLR |= 1u << 2;
    if (mode == HAL_POWER_STOP_WFI) wfi(); else wfe();
    PFIC_SCTLR &= ~(1u << 2);
    return ECOS_OK;
}
