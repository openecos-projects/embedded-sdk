#include "ecos/hal/interrupt.h"
#include "cl1_2512_soc.h"

#include <stddef.h>
#include <stdint.h>

#define MACHINE_STATUS_MIE             (1u << 3)
#define MACHINE_IE_MEIE                (1u << 11)
#define MACHINE_CAUSE_EXTERNAL         0x8000000Bu

typedef struct {
    hal_interrupt_handler_t handler;
    void *arg;
} interrupt_entry_t;

extern void ecos_cl1_2512_machine_trap_entry(void);

static interrupt_entry_t interrupt_entries[CL1_2512_PLIC_SOURCE_COUNT];
static uint8_t interrupt_initialized;

static uint32_t interrupt_lock(void)
{
    uint32_t previous;
    __asm__ volatile("csrrc %0, mstatus, %1"
                     : "=r"(previous)
                     : "r"(MACHINE_STATUS_MIE)
                     : "memory");
    return previous;
}

static void interrupt_unlock(uint32_t previous)
{
    if ((previous & MACHINE_STATUS_MIE) != 0u)
        __asm__ volatile("csrs mstatus, %0" : : "r"(MACHINE_STATUS_MIE)
                         : "memory");
}

static int interrupt_source_is_valid(uint32_t source)
{
    return source != 0u && source < CL1_2512_PLIC_SOURCE_COUNT;
}

__attribute__((weak, noreturn)) void hal_exception_handler(uint32_t mcause,
                                                            uint32_t mepc)
{
    (void)mcause;
    (void)mepc;
    hal_interrupt_global_disable();
    for (;;)
        __asm__ volatile("nop");
}

ecos_err_t hal_interrupt_init(void)
{
    uintptr_t entry;
    uint32_t previous;

    if (interrupt_initialized != 0u)
        return ECOS_OK;
    previous = interrupt_lock();
    REG_PLIC_ENABLE = 0u;
    REG_PLIC_THRESHOLD = 0u;
    for (uint32_t source = 0u; source < CL1_2512_PLIC_SOURCE_COUNT; ++source) {
        interrupt_entries[source].handler = NULL;
        interrupt_entries[source].arg = NULL;
    }
    entry = (uintptr_t)ecos_cl1_2512_machine_trap_entry;
    __asm__ volatile("csrw mtvec, %0" : : "r"(entry & ~(uintptr_t)0x3u)
                     : "memory");
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    interrupt_initialized = 1u;
    interrupt_unlock(previous);
    return ECOS_OK;
}

ecos_err_t hal_interrupt_alloc(uint32_t source,
                               uint32_t priority,
                               hal_interrupt_handler_t handler,
                               void *arg)
{
    uint32_t previous;

    if (!interrupt_source_is_valid(source) || priority == 0u || handler == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (hal_interrupt_init() != ECOS_OK)
        return ECOS_ERR_IO;
    previous = interrupt_lock();
    if (interrupt_entries[source].handler != NULL) {
        interrupt_unlock(previous);
        return ECOS_ERR_BUSY;
    }
    interrupt_entries[source].arg = arg;
    interrupt_entries[source].handler = handler;
    REG_PLIC_PRIORITY(source) = priority;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    interrupt_unlock(previous);
    return ECOS_OK;
}

ecos_err_t hal_interrupt_free(uint32_t source)
{
    uint32_t previous;

    if (!interrupt_source_is_valid(source))
        return ECOS_ERR_INVALID_ARGUMENT;
    previous = interrupt_lock();
    REG_PLIC_ENABLE &= ~(1u << source);
    REG_PLIC_PRIORITY(source) = 0u;
    interrupt_entries[source].handler = NULL;
    interrupt_entries[source].arg = NULL;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    interrupt_unlock(previous);
    return ECOS_OK;
}

ecos_err_t hal_interrupt_enable(uint32_t source)
{
    uint32_t previous;

    if (!interrupt_source_is_valid(source))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (interrupt_entries[source].handler == NULL)
        return ECOS_ERR_NOT_INITIALIZED;
    previous = interrupt_lock();
    REG_PLIC_ENABLE |= 1u << source;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    interrupt_unlock(previous);
    return ECOS_OK;
}

ecos_err_t hal_interrupt_disable(uint32_t source)
{
    uint32_t previous;

    if (!interrupt_source_is_valid(source))
        return ECOS_ERR_INVALID_ARGUMENT;
    previous = interrupt_lock();
    REG_PLIC_ENABLE &= ~(1u << source);
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    interrupt_unlock(previous);
    return ECOS_OK;
}

void hal_interrupt_set_threshold(uint32_t threshold)
{
    REG_PLIC_THRESHOLD = threshold;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
}

void hal_interrupt_global_enable(void)
{
    __asm__ volatile("csrs mie, %0" : : "r"(MACHINE_IE_MEIE) : "memory");
    __asm__ volatile("csrs mstatus, %0" : : "r"(MACHINE_STATUS_MIE)
                     : "memory");
}

void hal_interrupt_global_disable(void)
{
    __asm__ volatile("csrc mstatus, %0" : : "r"(MACHINE_STATUS_MIE)
                     : "memory");
    __asm__ volatile("csrc mie, %0" : : "r"(MACHINE_IE_MEIE) : "memory");
}

void ecos_cl1_2512_machine_trap_handler(uint32_t mcause, uint32_t mepc)
{
    hal_interrupt_handler_t handler;
    void *arg;
    uint32_t source;

    if (mcause != MACHINE_CAUSE_EXTERNAL)
        hal_exception_handler(mcause, mepc);
    source = REG_PLIC_CLAIM;
    if (!interrupt_source_is_valid(source))
        return;
    handler = interrupt_entries[source].handler;
    arg = interrupt_entries[source].arg;
    if (handler == NULL)
        REG_PLIC_ENABLE &= ~(1u << source);
    else
        handler(arg);
    REG_PLIC_COMPLETE = source;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
}
