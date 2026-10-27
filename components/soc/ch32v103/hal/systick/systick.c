/* CH32 PFIC SysTick: 64-bit compare value, 8 MHz/8 tick source as used by
 * the WCH examples. The generic interrupt backend supplies the ordinary RV32
 * software-context entry for IRQ12. */
#include "ecos/hal/systick.h"
#include "ecos/hal/interrupt.h"
#include "ch32v103_interrupt_internal.h"

#include <stddef.h>
#include <stdint.h>

#define SYSTICK_BASE 0xE000F000u
#define SYSTICK_CTLR  (*(volatile uint32_t *)(uintptr_t)(SYSTICK_BASE + 0x00u))
#define SYSTICK_CNTL0 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x04u))
#define SYSTICK_CNTL1 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x05u))
#define SYSTICK_CNTL2 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x06u))
#define SYSTICK_CNTL3 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x07u))
#define SYSTICK_CNTH0 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x08u))
#define SYSTICK_CNTH1 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x09u))
#define SYSTICK_CNTH2 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x0Au))
#define SYSTICK_CNTH3 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x0Bu))
#define SYSTICK_CMPL0 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x0Cu))
#define SYSTICK_CMPL1 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x0Du))
#define SYSTICK_CMPL2 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x0Eu))
#define SYSTICK_CMPL3 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x0Fu))
#define SYSTICK_CMPH0 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x10u))
#define SYSTICK_CMPH1 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x11u))
#define SYSTICK_CMPH2 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x12u))
#define SYSTICK_CMPH3 (*(volatile uint8_t *)(uintptr_t)(SYSTICK_BASE + 0x13u))
#define SYSTICK_ENABLE 0x01u

static hal_systick_callback_t callback;
static void *callback_arg;
static uint8_t configured;
static uint8_t irq_allocated;

static uint64_t read_counter(void)
{
    uint32_t high1;
    uint32_t low;
    uint32_t high2;
    do {
        high1 = (uint32_t)SYSTICK_CNTH0 | ((uint32_t)SYSTICK_CNTH1 << 8) |
                ((uint32_t)SYSTICK_CNTH2 << 16) | ((uint32_t)SYSTICK_CNTH3 << 24);
        low = (uint32_t)SYSTICK_CNTL0 | ((uint32_t)SYSTICK_CNTL1 << 8) |
              ((uint32_t)SYSTICK_CNTL2 << 16) | ((uint32_t)SYSTICK_CNTL3 << 24);
        high2 = (uint32_t)SYSTICK_CNTH0 | ((uint32_t)SYSTICK_CNTH1 << 8) |
                ((uint32_t)SYSTICK_CNTH2 << 16) | ((uint32_t)SYSTICK_CNTH3 << 24);
    } while (high1 != high2);
    return ((uint64_t)high2 << 32) | low;
}

static void clear_counter(void)
{
    SYSTICK_CTLR = 0u;
    SYSTICK_CNTL0 = 0u; SYSTICK_CNTL1 = 0u;
    SYSTICK_CNTL2 = 0u; SYSTICK_CNTL3 = 0u;
    SYSTICK_CNTH0 = 0u; SYSTICK_CNTH1 = 0u;
    SYSTICK_CNTH2 = 0u; SYSTICK_CNTH3 = 0u;
}

static void dispatch(void *arg)
{
    (void)arg;
    clear_counter();
    /* WCH SysTick is one-shot: the compare interrupt clears CTLR. Restart it
     * after clearing the current count, matching the vendor handler sequence. */
    SYSTICK_CTLR = SYSTICK_ENABLE;
    if (callback != NULL) callback(callback_arg);
}

ecos_err_t hal_systick_init(const hal_systick_config_t *config,
                            hal_systick_callback_t handler, void *arg,
                            uint32_t priority)
{
    ecos_err_t result;
    if (config == NULL || config->compare_ticks == 0u || handler == NULL ||
        priority == 0u || priority > CH32V103_IRQ_PRIORITY_MAX)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (irq_allocated != 0u) return ECOS_ERR_BUSY;
    clear_counter();
    SYSTICK_CMPL0 = (uint8_t)config->compare_ticks;
    SYSTICK_CMPL1 = (uint8_t)(config->compare_ticks >> 8);
    SYSTICK_CMPL2 = (uint8_t)(config->compare_ticks >> 16);
    SYSTICK_CMPL3 = (uint8_t)(config->compare_ticks >> 24);
    SYSTICK_CMPH0 = (uint8_t)(config->compare_ticks >> 32);
    SYSTICK_CMPH1 = (uint8_t)(config->compare_ticks >> 40);
    SYSTICK_CMPH2 = (uint8_t)(config->compare_ticks >> 48);
    SYSTICK_CMPH3 = (uint8_t)(config->compare_ticks >> 56);
    callback = handler;
    callback_arg = arg;
    result = hal_interrupt_alloc(CH32V103_IRQ_SYSTICK, priority, dispatch, NULL);
    if (result != ECOS_OK) return result;
    irq_allocated = 1u;
    configured = 1u;
    return ECOS_OK;
}

ecos_err_t hal_systick_start(void)
{
    if (configured == 0u) return ECOS_ERR_NOT_INITIALIZED;
    SYSTICK_CTLR = SYSTICK_ENABLE;
    return hal_interrupt_enable(CH32V103_IRQ_SYSTICK);
}

ecos_err_t hal_systick_stop(void)
{
    if (configured == 0u) return ECOS_ERR_NOT_INITIALIZED;
    SYSTICK_CTLR = 0u;
    (void)hal_interrupt_disable(CH32V103_IRQ_SYSTICK);
    return ECOS_OK;
}

ecos_err_t hal_systick_get_counter(uint64_t *counter)
{
    if (counter == NULL) return ECOS_ERR_INVALID_ARGUMENT;
    if (configured == 0u) return ECOS_ERR_NOT_INITIALIZED;
    *counter = read_counter();
    return ECOS_OK;
}
