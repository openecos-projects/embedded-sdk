/* CH32V103 independent watchdog backend.
 * The watchdog uses the LSI clock. The public driver expresses prescaler as
 * the hardware divider (4..256) and compare as the reload value (1..4096).
 * Enabling the watchdog is intentionally irreversible until reset, matching
 * the CH32 IWDG hardware behavior.
 */
#include "ecos/hal/wdg.h"

#include <stddef.h>
#include <stdint.h>

#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))

#define RCC_RSTSCKR REG32(0x40021024u)
#define RCC_LSION   0x00000001u
#define RCC_LSIRDY  0x00000002u
#define RCC_RMVF    0x01000000u
#define RCC_IWDGRSTF 0x20000000u
#define RCC_WWDGRSTF 0x40000000u
#define IWDG_BASE   0x40003000u
#define IWDG_CTLR   REG32(IWDG_BASE + 0x00u)
#define IWDG_PSCR   REG32(IWDG_BASE + 0x04u)
#define IWDG_RLDR   REG32(IWDG_BASE + 0x08u)
#define IWDG_STATR  REG32(IWDG_BASE + 0x0Cu)

#define IWDG_KEY_ACCESS 0x5555u
#define IWDG_KEY_RELOAD 0xAAAAu
#define IWDG_KEY_ENABLE 0xCCCCu
#define IWDG_STATUS_PVU 0x0001u
#define IWDG_STATUS_RVU 0x0002u
#define IWDG_RESET_FLAG  RCC_IWDGRSTF

static uint16_t prescaler_code(uint32_t prescaler)
{
    switch (prescaler) {
    case 4u:   return 0u;
    case 8u:   return 1u;
    case 16u:  return 2u;
    case 32u:  return 3u;
    case 64u:  return 4u;
    case 128u: return 5u;
    case 256u: return 6u;
    default:   return 0xFFFFu;
    }
}

static int valid_id(hal_wdg_id_t id)
{
    return id == HAL_WDG_ID_0;
}

ecos_err_t hal_wdg_init(hal_wdg_id_t id, const hal_wdg_config_t *config)
{
    uint16_t code;

    if (!valid_id(id) || config == NULL || config->prescaler == 0u ||
        config->compare == 0u || config->compare > 4096u)
        return ECOS_ERR_INVALID_ARGUMENT;
    code = prescaler_code(config->prescaler);
    if (code == 0xFFFFu)
        return ECOS_ERR_INVALID_ARGUMENT;

    /* IWDG is clocked from LSI; wait for the actual LSI ready flag in
     * RSTSCKR, not an unrelated reset/status bit. */
    RCC_RSTSCKR |= RCC_LSION;
    {
        uint32_t timeout = 200000u;
        while ((RCC_RSTSCKR & RCC_LSIRDY) == 0u) {
            if (timeout-- == 0u)
                return ECOS_ERR_TIMEOUT;
        }
    }
    /* The CH32 IWDG cannot be disabled after IWDG_KEY_ENABLE. */
    IWDG_CTLR = IWDG_KEY_ACCESS;
    IWDG_PSCR = code;
    IWDG_RLDR = (uint16_t)(config->compare - 1u);
    IWDG_CTLR = IWDG_KEY_RELOAD;
    IWDG_CTLR = IWDG_KEY_ENABLE;

    {
        uint32_t timeout = 200000u;
        while ((IWDG_STATR & (IWDG_STATUS_PVU | IWDG_STATUS_RVU)) != 0u) {
            if (timeout-- == 0u)
                return ECOS_ERR_TIMEOUT;
        }
    }
    (void)config->control; /* hardware enable is represented by KEY_ENABLE */
    return ECOS_OK;
}

ecos_err_t hal_wdg_deinit(hal_wdg_id_t id)
{
    /* CH32 IWDG has no disable operation before reset. */
    if (!valid_id(id))
        return ECOS_ERR_INVALID_ARGUMENT;
    return ECOS_ERR_UNSUPPORTED;
}

ecos_err_t hal_wdg_feed(hal_wdg_id_t id)
{
    if (!valid_id(id))
        return ECOS_ERR_INVALID_ARGUMENT;
    IWDG_CTLR = IWDG_KEY_RELOAD;
    return ECOS_OK;
}

ecos_err_t hal_wdg_get_status(hal_wdg_id_t id, uint32_t *status)
{
    if (!valid_id(id) || status == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    *status = (uint32_t)IWDG_STATR;
    return ECOS_OK;
}

/* Exported helpers for board reset diagnostics.  Reading RSTSCKR must not be
 * confused with the LSI-ready flag in RCC_CTLR. */
uint32_t ch32v103_reset_flags(void)
{
    return RCC_RSTSCKR & (RCC_IWDGRSTF | RCC_WWDGRSTF);
}

void ch32v103_clear_reset_flags(void)
{
    RCC_RSTSCKR |= RCC_RMVF;
}
