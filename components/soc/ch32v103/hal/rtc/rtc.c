/* CH32V103 RTC counter backend using the LSI clock.
 * The RTC is a free-running 32-bit counter with alarm compare; it is not a
 * calendar clock. Backup-domain initialization may reset retained state.
 */
#include "ecos/hal/rtc.h"

#include <stddef.h>
#include <stdint.h>

#define REG16(address) (*(volatile uint16_t *)(uintptr_t)(address))
#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define RCC_RSTSCKR   REG32(0x40021024u)
#define RCC_APB1PCENR REG32(0x4002101Cu)
#define RCC_BDCTLR    REG32(0x40021020u)
#define PWR_CTLR      REG32(0x40007000u)
#define RTC_BASE      0x40002800u
/* Every 16-bit RTC register is followed by one reserved half-word. */
#define RTC_CTLRH     REG16(RTC_BASE + 0x00u)
#define RTC_CTLRL     REG16(RTC_BASE + 0x04u)
#define RTC_PSCRH     REG16(RTC_BASE + 0x08u)
#define RTC_PSCRL     REG16(RTC_BASE + 0x0Cu)
#define RTC_DIVH      REG16(RTC_BASE + 0x10u)
#define RTC_DIVL      REG16(RTC_BASE + 0x14u)
#define RTC_CNTH      REG16(RTC_BASE + 0x18u)
#define RTC_CNTL      REG16(RTC_BASE + 0x1Cu)
#define RTC_ALRMH     REG16(RTC_BASE + 0x20u)
#define RTC_ALRML     REG16(RTC_BASE + 0x24u)

#define RCC_CTLR_LSION  0x00000001u
#define RCC_CTLR_LSIRDY 0x00000002u
#define RCC_APB1_PWREN  0x10000000u
#define RCC_APB1_BKPEN  0x08000000u
#define PWR_DBP         0x00000100u
#define BDCTLR_RTCSEL_LSI 0x00000200u
#define BDCTLR_RTCEN    0x00008000u
#define RTC_CNF         0x0010u
#define RTC_RTOFF       0x0020u
#define RTC_RSF         0x0008u
#define RTC_ALRIE       0x0002u
#define RTC_ALRF        0x0002u
#define RTC_TIMEOUT     200000u
#define RTC_PRESCALER_MAX 0x100000u

static int valid_id(hal_rtc_id_t id)
{
    return id == HAL_RTC_ID_0;
}

static ecos_err_t wait_rtoff(void)
{
    uint32_t timeout = RTC_TIMEOUT;
    while ((RTC_CTLRL & RTC_RTOFF) == 0u) {
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    return ECOS_OK;
}

static ecos_err_t wait_sync(void)
{
    uint32_t timeout = RTC_TIMEOUT;

    RTC_CTLRL &= (uint16_t)~RTC_RSF;
    while ((RTC_CTLRL & RTC_RSF) == 0u) {
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    return ECOS_OK;
}

static void enter_config(void)
{
    RTC_CTLRL |= RTC_CNF;
}

static void leave_config(void)
{
    RTC_CTLRL &= (uint16_t)~RTC_CNF;
}

static uint32_t read_counter(void)
{
    uint16_t high1;
    uint16_t low;
    uint16_t high2;
    do {
        high1 = RTC_CNTH;
        low = RTC_CNTL;
        high2 = RTC_CNTH;
    } while (high1 != high2);
    return ((uint32_t)high2 << 16) | low;
}

ecos_err_t hal_rtc_init(hal_rtc_id_t id, const hal_rtc_config_t *config)
{
    uint32_t divider;
    ecos_err_t result;

    if (!valid_id(id) || config == NULL || config->prescaler == 0u ||
        config->prescaler > RTC_PRESCALER_MAX)
        return ECOS_ERR_INVALID_ARGUMENT;
    RCC_RSTSCKR |= RCC_CTLR_LSION;
    {
        uint32_t timeout = RTC_TIMEOUT;
        while ((RCC_RSTSCKR & RCC_CTLR_LSIRDY) == 0u) {
            if (timeout-- == 0u)
                return ECOS_ERR_TIMEOUT;
        }
    }
    RCC_APB1PCENR |= RCC_APB1_PWREN | RCC_APB1_BKPEN;
    PWR_CTLR |= PWR_DBP;
    /* Do not reset the backup domain on every init: that would destroy a
     * retained counter.  Reject a different already-selected RTC source. */
    if ((RCC_BDCTLR & 0x0300u) == 0u) {
        RCC_BDCTLR |= BDCTLR_RTCSEL_LSI | BDCTLR_RTCEN;
    } else if ((RCC_BDCTLR & 0x0300u) != BDCTLR_RTCSEL_LSI ||
               (RCC_BDCTLR & BDCTLR_RTCEN) == 0u) {
        return ECOS_ERR_INVALID_STATE;
    }
    result = wait_rtoff();
    if (result != ECOS_OK)
        return result;
    result = wait_sync();
    if (result != ECOS_OK)
        return result;
    divider = config->prescaler - 1u;
    enter_config();
    RTC_PSCRH = (uint16_t)(divider >> 16);
    RTC_PSCRL = (uint16_t)divider;
    RTC_CTLRH = config->alarm_enabled != 0u ? RTC_ALRIE : 0u;
    leave_config();
    return wait_rtoff();
}

ecos_err_t hal_rtc_deinit(hal_rtc_id_t id)
{
    if (!valid_id(id))
        return ECOS_ERR_INVALID_ARGUMENT;
    return ECOS_ERR_UNSUPPORTED;
}

ecos_err_t hal_rtc_get_counter(hal_rtc_id_t id, uint32_t *counter)
{
    if (!valid_id(id) || counter == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    *counter = read_counter();
    return ECOS_OK;
}

ecos_err_t hal_rtc_set_counter(hal_rtc_id_t id, uint32_t counter)
{
    ecos_err_t result;
    if (!valid_id(id))
        return ECOS_ERR_INVALID_ARGUMENT;
    result = wait_rtoff();
    if (result != ECOS_OK)
        return result;
    enter_config();
    RTC_CNTH = (uint16_t)(counter >> 16);
    RTC_CNTL = (uint16_t)counter;
    leave_config();
    return wait_rtoff();
}

ecos_err_t hal_rtc_set_alarm(hal_rtc_id_t id, uint32_t alarm)
{
    ecos_err_t result;
    if (!valid_id(id))
        return ECOS_ERR_INVALID_ARGUMENT;
    result = wait_rtoff();
    if (result != ECOS_OK)
        return result;
    enter_config();
    RTC_ALRMH = (uint16_t)(alarm >> 16);
    RTC_ALRML = (uint16_t)alarm;
    leave_config();
    return wait_rtoff();
}

int hal_rtc_alarm_triggered(hal_rtc_id_t id)
{
    if (!valid_id(id))
        return ECOS_ERR_INVALID_ARGUMENT;
    return (RTC_CTLRL & RTC_ALRF) != 0u;
}
