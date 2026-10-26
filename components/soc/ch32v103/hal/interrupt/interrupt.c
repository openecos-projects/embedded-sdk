/* CH32V103 PFIC backend, ordinary RV32 software context (no WCH fast ABI).
 * Register map and CFGR keys checked against openwch/ch32v103 EVT core_riscv.h
 * at ff367d8fec5c9f40029355b2806cd059d9508e4e. Not a PLIC implementation.
 */
#include "ecos/hal/interrupt.h"
#include "ch32v103_interrupt_internal.h"

#include <stddef.h>

#define PFIC_KEY_CONFIG 0xFA050000u
#define PFIC_NO_HARDWARE_STACK (1u << 0)
#define PFIC_NO_NESTING (1u << 1)
#define PFIC_MASK_ALL 0x10u
#define MCAUSE_INTERRUPT 0x80000000u

extern const uint8_t ecos_ch32v103_vector_table[];

typedef struct {
    hal_interrupt_handler_t handler;
    void *arg;
    uint8_t priority;
    uint8_t enabled;
} irq_entry_t;

static irq_entry_t entries[CH32V103_IRQ_COUNT];
static uint32_t threshold;
static uint8_t initialized;
volatile ch32v103_fault_t ecos_ch32v103_last_fault;

int ch32v103_irq_source_valid(uint32_t source)
{
    return source == CH32V103_IRQ_SYSTICK ||
           source == CH32V103_IRQ_SOFTWARE ||
           (source >= 16u && source <= 34u) ||
           (source >= 39u && source < CH32V103_IRQ_COUNT);
}

static uint32_t irq_bit(uint32_t source)
{
    return 1u << (source & 31u);
}

static void irq_disable_hw(uint32_t source)
{
    /* Follow WCH's IRER sequence: mask arbitration while disabling a source. */
    uint32_t previous = ch32v103_read32(CH32V103_PFIC_THRESHOLD);
    ch32v103_write32(CH32V103_PFIC_THRESHOLD, PFIC_MASK_ALL);
    ch32v103_write32(CH32V103_PFIC_DISABLE(source >> 5), irq_bit(source));
    ch32v103_write32(CH32V103_PFIC_THRESHOLD, previous);
    ch32v103_io_fence();
}

static void irq_clear_pending(uint32_t source)
{
    ch32v103_write32(CH32V103_PFIC_CLEAR_PENDING(source >> 5), irq_bit(source));
}

static void irq_apply_enable(uint32_t source)
{
    if (entries[source].enabled != 0u && entries[source].priority > threshold)
        ch32v103_write32(CH32V103_PFIC_ENABLE(source >> 5), irq_bit(source));
    else
        irq_disable_hw(source);
    ch32v103_io_fence();
}

ecos_err_t hal_interrupt_init(void)
{
    uint32_t state = ch32v103_irq_lock();
    if (initialized == 0u) {
        /* No private WCH compiler attributes or hardware register stacking.
         * All handlers run non-nested; the assembly entry saves x1/x3..x31. */
        ch32v103_write32(CH32V103_PFIC_CFGR,
                         PFIC_KEY_CONFIG | PFIC_NO_HARDWARE_STACK | PFIC_NO_NESTING);
        ch32v103_write32(CH32V103_PFIC_THRESHOLD, PFIC_MASK_ALL);
        for (uint32_t word = 0u; word < 2u; ++word) {
            /* IRQ12/14, IRQ16..34, IRQ39..59 only; do not touch NMI or holes. */
            uint32_t mask = word == 0u ? 0xFFFF5000u : 0x0FFFFF87u;
            ch32v103_write32(CH32V103_PFIC_DISABLE(word), mask);
            ch32v103_write32(CH32V103_PFIC_CLEAR_PENDING(word), mask);
        }
        ch32v103_write32(CH32V103_PFIC_THRESHOLD, 0u);
        /* Disable VTF bypasses left by a loader/debugger. */
        for (uint32_t slot = 0u; slot < 4u; ++slot)
            ch32v103_write32(CH32V103_PFIC_BASE + 0x60u + 4u * slot, 0u);
        threshold = 0u;
        ch32v103_install_mtvec((uintptr_t)ecos_ch32v103_vector_table | 1u);
        ch32v103_io_fence();
        initialized = 1u;
    }
    ch32v103_irq_restore(state);
    return ECOS_OK;
}

ecos_err_t hal_interrupt_alloc(uint32_t source, uint32_t priority,
                               hal_interrupt_handler_t handler, void *arg)
{
    uint32_t state;
    if (!ch32v103_irq_source_valid(source) || handler == NULL || priority == 0u ||
        priority > CH32V103_IRQ_PRIORITY_MAX)
        return ECOS_ERR_INVALID_ARGUMENT;
    (void)hal_interrupt_init();
    state = ch32v103_irq_lock();
    if (entries[source].handler != NULL) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_BUSY;
    }
    irq_disable_hw(source);
    irq_clear_pending(source);
    ch32v103_write8(CH32V103_PFIC_PRIORITY(source),
                    (uint8_t)((CH32V103_IRQ_PRIORITY_MAX - priority) << 4));
    entries[source].arg = arg;
    entries[source].priority = (uint8_t)priority;
    entries[source].enabled = 0u;
    entries[source].handler = handler;
    ch32v103_irq_restore(state);
    return ECOS_OK;
}

ecos_err_t hal_interrupt_free(uint32_t source)
{
    uint32_t state;
    if (!ch32v103_irq_source_valid(source))
        return ECOS_ERR_INVALID_ARGUMENT;
    state = ch32v103_irq_lock();
    if (entries[source].handler == NULL) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_NOT_INITIALIZED;
    }
    irq_disable_hw(source);
    irq_clear_pending(source);
    entries[source].enabled = 0u;
    entries[source].handler = NULL;
    entries[source].arg = NULL;
    entries[source].priority = 0u;
    ch32v103_irq_restore(state);
    return ECOS_OK;
}

ecos_err_t hal_interrupt_enable(uint32_t source)
{
    uint32_t state;
    if (!ch32v103_irq_source_valid(source))
        return ECOS_ERR_INVALID_ARGUMENT;
    state = ch32v103_irq_lock();
    if (entries[source].handler == NULL) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_NOT_INITIALIZED;
    }
    entries[source].enabled = 1u;
    irq_apply_enable(source);
    ch32v103_irq_restore(state);
    return ECOS_OK;
}

ecos_err_t hal_interrupt_disable(uint32_t source)
{
    uint32_t state;
    if (!ch32v103_irq_source_valid(source))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    state = ch32v103_irq_lock();
    entries[source].enabled = 0u;
    irq_disable_hw(source);
    ch32v103_irq_restore(state);
    return ECOS_OK;
}

void hal_interrupt_set_threshold(uint32_t value)
{
    uint32_t state;
    (void)hal_interrupt_init();
    state = ch32v103_irq_lock();
    threshold = value;
    /* Apply the public, larger-is-more-urgent threshold convention using
     * enables, rather than exposing PFIC's reverse-encoded hardware field. */
    for (uint32_t source = 0u; source < CH32V103_IRQ_COUNT; ++source) {
        if (entries[source].handler != NULL)
            irq_apply_enable(source);
    }
    ch32v103_irq_restore(state);
}

void hal_interrupt_global_enable(void)
{
    (void)hal_interrupt_init();
    ch32v103_io_fence();
    ch32v103_cpu_irq_enable();
}

void hal_interrupt_global_disable(void)
{
    ch32v103_cpu_irq_disable();
    ch32v103_io_fence();
}

__attribute__((weak, noreturn))
void ecos_ch32v103_exception_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval)
{
    (void)mcause;
    (void)mepc;
    (void)mtval;
    for (;;)
        ;
}

void ecos_ch32v103_trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval)
{
    uint32_t source = mcause & ~MCAUSE_INTERRUPT;
    if ((mcause & MCAUSE_INTERRUPT) == 0u || !ch32v103_irq_source_valid(source)) {
        hal_interrupt_global_disable();
        ecos_ch32v103_last_fault.mcause = mcause;
        ecos_ch32v103_last_fault.mepc = mepc;
        ecos_ch32v103_last_fault.mtval = mtval;
        ecos_ch32v103_exception_handler(mcause, mepc, mtval);
    }
    /* No PLIC claim/complete exists: each peripheral callback clears its own
     * event flag. Mask unexpected sources to prevent an interrupt storm. */
    if (entries[source].handler != NULL && entries[source].enabled != 0u &&
        entries[source].priority > threshold) {
        entries[source].handler(entries[source].arg);
    } else {
        irq_disable_hw(source);
        irq_clear_pending(source);
    }
}
