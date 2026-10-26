#ifndef CH32V103_INTERRUPT_INTERNAL_H
#define CH32V103_INTERRUPT_INTERNAL_H

/* Target-private register/CPU access. Host tests supply a register model at
 * this boundary; application code must use the public Driver API instead. */
#include <stdint.h>

#define CH32V103_IRQ_COUNT 60u
#define CH32V103_IRQ_SYSTICK 12u
#define CH32V103_IRQ_SOFTWARE 14u
#define CH32V103_IRQ_EXTI0 22u
#define CH32V103_IRQ_EXTI9_5 39u
#define CH32V103_IRQ_TIM2 44u
#define CH32V103_IRQ_EXTI15_10 56u
#define CH32V103_IRQ_PRIORITY_MAX 16u
#define CH32V103_MSTATUS_IRQ_ENABLE 0x88u

#define CH32V103_PFIC_BASE 0xE000E000u
#define CH32V103_PFIC_THRESHOLD (CH32V103_PFIC_BASE + 0x040u)
#define CH32V103_PFIC_CFGR (CH32V103_PFIC_BASE + 0x048u)
#define CH32V103_PFIC_ENABLE(word) (CH32V103_PFIC_BASE + 0x100u + 4u * (word))
#define CH32V103_PFIC_DISABLE(word) (CH32V103_PFIC_BASE + 0x180u + 4u * (word))
#define CH32V103_PFIC_CLEAR_PENDING(word) (CH32V103_PFIC_BASE + 0x280u + 4u * (word))
#define CH32V103_PFIC_PRIORITY(source) (CH32V103_PFIC_BASE + 0x400u + (source))

#ifdef ECOS_CH32V103_INTERRUPT_TEST
uint8_t ch32v103_read8(uintptr_t address);
uint16_t ch32v103_read16(uintptr_t address);
uint32_t ch32v103_read32(uintptr_t address);
void ch32v103_write8(uintptr_t address, uint8_t value);
void ch32v103_write16(uintptr_t address, uint16_t value);
void ch32v103_write32(uintptr_t address, uint32_t value);
uint32_t ch32v103_irq_lock(void);
void ch32v103_irq_restore(uint32_t state);
void ch32v103_cpu_irq_enable(void);
void ch32v103_cpu_irq_disable(void);
void ch32v103_install_mtvec(uintptr_t entry);
void ch32v103_io_fence(void);
#else
static inline uint8_t ch32v103_read8(uintptr_t address)
{
    return *(volatile uint8_t *)address;
}
static inline uint16_t ch32v103_read16(uintptr_t address)
{
    return *(volatile uint16_t *)address;
}
static inline uint32_t ch32v103_read32(uintptr_t address)
{
    return *(volatile uint32_t *)address;
}
static inline void ch32v103_write8(uintptr_t address, uint8_t value)
{
    *(volatile uint8_t *)address = value;
}
static inline void ch32v103_write16(uintptr_t address, uint16_t value)
{
    *(volatile uint16_t *)address = value;
}
static inline void ch32v103_write32(uintptr_t address, uint32_t value)
{
    *(volatile uint32_t *)address = value;
}
static inline uint32_t ch32v103_irq_lock(void)
{
    uint32_t state;
    __asm__ volatile("csrrc %0, mstatus, %1"
                     : "=r"(state)
                     : "r"(CH32V103_MSTATUS_IRQ_ENABLE) : "memory");
    return state;
}
static inline void ch32v103_irq_restore(uint32_t state)
{
    __asm__ volatile("csrw mstatus, %0" : : "r"(state) : "memory");
}
static inline void ch32v103_cpu_irq_enable(void)
{
    __asm__ volatile("csrs mstatus, %0"
                     : : "r"(CH32V103_MSTATUS_IRQ_ENABLE) : "memory");
}
static inline void ch32v103_cpu_irq_disable(void)
{
    __asm__ volatile("csrc mstatus, %0"
                     : : "r"(CH32V103_MSTATUS_IRQ_ENABLE) : "memory");
}
static inline void ch32v103_install_mtvec(uintptr_t entry)
{
    __asm__ volatile("csrw mtvec, %0" : : "r"(entry) : "memory");
}
static inline void ch32v103_io_fence(void)
{
    __asm__ volatile("fence iorw, iorw" : : : "memory");
}
#endif

/* Larger public priority is more urgent (1..16); PFIC encoding is reversed.
 * Public threshold masks priorities <= threshold; 0 permits every priority.
 * These helpers reject holes in the V103 IRQ map, not just out-of-range IDs. */
int ch32v103_irq_source_valid(uint32_t source);
void ecos_ch32v103_trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval);

/* Inspectable in a debugger; no serial I/O is attempted in exception context.
 * A user hook may override the weak, non-returning exception handler. */
typedef struct {
    uint32_t mcause;
    uint32_t mepc;
    uint32_t mtval;
} ch32v103_fault_t;
extern volatile ch32v103_fault_t ecos_ch32v103_last_fault;
void ecos_ch32v103_exception_handler(uint32_t mcause, uint32_t mepc,
                                    uint32_t mtval) __attribute__((noreturn));

#endif
