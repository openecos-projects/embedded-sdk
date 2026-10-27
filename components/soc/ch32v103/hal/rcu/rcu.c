/* CH32V103 RCC clock/reset backend.
 * The target boots from HSI at 8 MHz.  This backend exposes status and a
 * conservative HSI/AHB/APB divider configuration; PLL/HSE switching is kept
 * out of the capability list until a board crystal and clock contract are
 * selected explicitly.
 */
#include "ecos/hal/rcu.h"

#include <stddef.h>
#include <stdint.h>

#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define RCC_CTLR       REG32(0x40021000u)
#define RCC_CFGR0      REG32(0x40021004u)

#define RCC_HSION      0x00000001u
#define RCC_HSIRDY     0x00000002u
#define RCC_SW         0x00000003u
#define RCC_SW_HSI     0x00000000u
#define RCC_SWS_HSI    0x00000000u
#define RCC_HPRE       0x000000F0u
#define RCC_PPRE1      0x00000700u
#define RCC_PPRE2      0x00003800u
#define RCC_ADCPRE     0x0000C000u
#define RCC_TIMEOUT    200000u
#define CH32V103_HSI_HZ 8000000u

static uint8_t initialized;
static uint32_t ahb_hz = CH32V103_HSI_HZ;
static uint32_t apb1_hz = CH32V103_HSI_HZ;
static uint32_t apb2_hz = CH32V103_HSI_HZ;

static int valid_id(hal_rcu_id_t id)
{
    return id == HAL_RCU_ID_0;
}

static uint32_t divider_from_hpre(uint32_t value)
{
    static const uint16_t dividers[16] = {
        1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
        2u, 4u, 8u, 16u, 64u, 128u, 256u, 512u
    };
    return dividers[(value >> 4) & 0xFu];
}

static uint32_t divider_from_ppre(uint32_t value, uint32_t shift)
{
    static const uint8_t dividers[8] = { 1u, 1u, 1u, 1u, 2u, 4u, 8u, 16u };
    return dividers[(value >> shift) & 7u];
}

static ecos_err_t wait_hsi(void)
{
    uint32_t timeout = RCC_TIMEOUT;
    RCC_CTLR |= RCC_HSION;
    while ((RCC_CTLR & RCC_HSIRDY) == 0u) {
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    return ECOS_OK;
}

static void update_frequencies(void)
{
    uint32_t hclk_div = divider_from_hpre(RCC_CFGR0 & RCC_HPRE);
    uint32_t pclk1_div = divider_from_ppre(RCC_CFGR0, 8u);
    uint32_t pclk2_div = divider_from_ppre(RCC_CFGR0, 11u);

    ahb_hz = CH32V103_HSI_HZ / hclk_div;
    apb1_hz = ahb_hz / pclk1_div;
    apb2_hz = ahb_hz / pclk2_div;
}

ecos_err_t hal_rcu_init(hal_rcu_id_t id, const hal_rcu_config_t *config)
{
    ecos_err_t result;
    uint32_t divider;

    if (!valid_id(id) || config == NULL || config->clock_divider == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (config->control != 0u)
        return ECOS_ERR_UNSUPPORTED;
    /* clock_divider is the desired HSI-derived AHB divider. */
    divider = config->clock_divider;
    if (divider != 1u && divider != 2u && divider != 4u && divider != 8u &&
        divider != 16u && divider != 64u && divider != 128u &&
        divider != 256u && divider != 512u)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = wait_hsi();
    if (result != ECOS_OK)
        return result;
    /* No PLL/HSE assumption: select HSI before changing dividers. */
    RCC_CFGR0 = (RCC_CFGR0 & ~(RCC_SW | RCC_HPRE | RCC_PPRE1 | RCC_PPRE2 |
                                RCC_ADCPRE)) | RCC_SW_HSI;
    switch (divider) {
    case 1u: break;
    case 2u: RCC_CFGR0 |= 0x80u; break;
    case 4u: RCC_CFGR0 |= 0x90u; break;
    case 8u: RCC_CFGR0 |= 0xA0u; break;
    case 16u: RCC_CFGR0 |= 0xB0u; break;
    case 64u: RCC_CFGR0 |= 0xC0u; break;
    case 128u: RCC_CFGR0 |= 0xD0u; break;
    case 256u: RCC_CFGR0 |= 0xE0u; break;
    default: RCC_CFGR0 |= 0xF0u; break;
    }
    /* Keep APB buses at the selected AHB frequency and use a legal ADC clock. */
    RCC_CFGR0 |= 0x0000C000u;
    update_frequencies();
    initialized = 1u;
    (void)config->control; /* reserved for future reset-domain policy */
    return ECOS_OK;
}

ecos_err_t hal_rcu_deinit(hal_rcu_id_t id)
{
    if (!valid_id(id)) return ECOS_ERR_INVALID_ARGUMENT;
    initialized = 0u;
    return ECOS_OK;
}

ecos_err_t hal_rcu_get_status(hal_rcu_id_t id, uint32_t *status)
{
    if (!valid_id(id) || status == NULL) return ECOS_ERR_INVALID_ARGUMENT;
    *status = RCC_CFGR0;
    return ECOS_OK;
}

ecos_err_t hal_rcu_pll_apply(hal_rcu_id_t id,
                             const hal_rcu_pll_config_t *config)
{
    (void)id;
    (void)config;
    return ECOS_ERR_UNSUPPORTED;
}

ecos_err_t hal_rcu_pll_get_freq(hal_rcu_id_t id,
                                 hal_rcu_clock_domain_t domain,
                                 uint32_t *freq_hz)
{
    if (!valid_id(id) || freq_hz == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if ((unsigned)domain > (unsigned)HAL_RCU_CLOCK_APB)
        return ECOS_ERR_INVALID_ARGUMENT;
    /* CH32V103 currently exposes only the conservative HSI divider backend;
     * no public rcu-pll capability is declared for this target. */
    (void)freq_hz;
    return ECOS_ERR_UNSUPPORTED;
}

uint32_t ch32v103_get_ahb_clock_hz(void) { return ahb_hz; }
uint32_t ch32v103_get_apb1_clock_hz(void) { return apb1_hz; }
uint32_t ch32v103_get_apb2_clock_hz(void) { return apb2_hz; }
