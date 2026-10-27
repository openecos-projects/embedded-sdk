/* CH32V103 window watchdog.  Once enabled the WWDG cannot be stopped until
 * reset; deinit therefore returns ECOS_ERR_UNSUPPORTED after init. */
#include "ecos/hal/wwdg.h"

#include <stddef.h>
#include <stdint.h>

#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define RCC_APB1PCENR REG32(0x4002101Cu)
#define WWDG_BASE     0x40002C00u
#define WWDG_CTLR     REG32(WWDG_BASE + 0x00u)
#define WWDG_CFGR     REG32(WWDG_BASE + 0x04u)
#define WWDG_STATR    REG32(WWDG_BASE + 0x08u)
#define RCC_WWDGEN   (1u << 11)
#define WWDG_WDGA    0x80u
#define WWDG_EWI     (1u << 9)

static uint8_t initialized;

static int valid_id(hal_wwdg_id_t id) { return id == HAL_WWDG_ID_0; }
static uint32_t prescaler_bits(uint32_t value)
{
    switch (value) {
    case 1u: return 0u;
    case 2u: return 0x80u;
    case 4u: return 0x100u;
    case 8u: return 0x180u;
    default: return UINT32_MAX;
    }
}

ecos_err_t hal_wwdg_init(hal_wwdg_id_t id, const hal_wwdg_config_t *config)
{
    uint32_t prescaler;

    if (!valid_id(id) || config == NULL || config->window < 0x40u ||
        config->window > 0x7Fu || config->counter < 0x40u ||
        config->counter > 0x7Fu || config->counter <= config->window)
        return ECOS_ERR_INVALID_ARGUMENT;
    prescaler = prescaler_bits(config->prescaler);
    if (prescaler == UINT32_MAX) return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized != 0u) return ECOS_ERR_BUSY;
    RCC_APB1PCENR |= RCC_WWDGEN;
    WWDG_CFGR = prescaler | config->window |
                (config->interrupt_enable != 0u ? WWDG_EWI : 0u);
    WWDG_CTLR = WWDG_WDGA | config->counter;
    initialized = 1u;
    return ECOS_OK;
}

ecos_err_t hal_wwdg_deinit(hal_wwdg_id_t id)
{
    if (!valid_id(id)) return ECOS_ERR_INVALID_ARGUMENT;
    return initialized != 0u ? ECOS_ERR_UNSUPPORTED : ECOS_OK;
}

ecos_err_t hal_wwdg_feed(hal_wwdg_id_t id, uint8_t counter)
{
    if (!valid_id(id) || counter < 0x40u || counter > 0x7Fu)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u) return ECOS_ERR_NOT_INITIALIZED;
    /* The hardware compares its current down-counter against WINDOW; the
     * value written here is the reload value and may be above WINDOW. */
    WWDG_CTLR = WWDG_WDGA | counter;
    return ECOS_OK;
}

ecos_err_t hal_wwdg_get_status(hal_wwdg_id_t id, uint32_t *status)
{
    if (!valid_id(id) || status == NULL) return ECOS_ERR_INVALID_ARGUMENT;
    *status = WWDG_STATR;
    return ECOS_OK;
}

ecos_err_t hal_wwdg_clear_status(hal_wwdg_id_t id)
{
    if (!valid_id(id)) return ECOS_ERR_INVALID_ARGUMENT;
    WWDG_STATR = 0u;
    return ECOS_OK;
}
