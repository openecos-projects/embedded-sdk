/* Host register model: validates ECOS state/ownership/register sequences,
 * not the physical PFIC arbitration, CSR instructions, or trap execution. */
#include "ecos/driver/gpio_interrupt.h"
#include "ecos/driver/interrupt.h"
#include "ecos/driver/timer.h"
#include "ecos/driver/timer_interrupt.h"
#include "ecos/hal/timer_interrupt.h"
#include "ch32v103_timer_internal.h"

#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define EXTI_INTENR 0x40010400u
#define EXTI_RTENR 0x40010408u
#define EXTI_FTENR 0x4001040Cu
#define EXTI_INTFR 0x40010414u
#define AFIO_EXTICR(index) (0x40010008u + 4u * (index))

typedef struct {
    uintptr_t address;
    uint32_t value;
} mock_register_t;

static mock_register_t registers[128];
static size_t register_count;
static uint32_t enabled[2];
static uint32_t pending[2];
static uint32_t cpu_mstatus = 0x1888u;
static uintptr_t mtvec;
static unsigned vector_installs;
static unsigned fences;
static jmp_buf fault_return;
const uint8_t ecos_ch32v103_vector_table[240] __attribute__((aligned(256))) = { 0u };

static uint32_t *reg(uintptr_t address)
{
    for (size_t index = 0u; index < register_count; ++index) {
        if (registers[index].address == address)
            return &registers[index].value;
    }
    assert(register_count < sizeof(registers) / sizeof(registers[0]));
    registers[register_count].address = address;
    return &registers[register_count++].value;
}

uint32_t ch32v103_read32(uintptr_t address) { return *reg(address); }
uint16_t ch32v103_read16(uintptr_t address) { return (uint16_t)*reg(address); }
uint8_t ch32v103_read8(uintptr_t address) { return (uint8_t)*reg(address); }
void ch32v103_write8(uintptr_t address, uint8_t value) { *reg(address) = value; }
void ch32v103_write32(uintptr_t address, uint32_t value)
{
    for (uint32_t word = 0u; word < 2u; ++word) {
        if (address == CH32V103_PFIC_ENABLE(word)) {
            enabled[word] |= value;
            return;
        }
        if (address == CH32V103_PFIC_DISABLE(word)) {
            assert(ch32v103_read32(CH32V103_PFIC_THRESHOLD) == 0x10u);
            enabled[word] &= ~value;
            return;
        }
        if (address == CH32V103_PFIC_CLEAR_PENDING(word)) {
            pending[word] &= ~value;
            return;
        }
    }
    if (address == EXTI_INTFR)
        *reg(address) &= ~value; /* W1C */
    else
        *reg(address) = value;
}
void ch32v103_write16(uintptr_t address, uint16_t value)
{
    if (address == CH32V103_TIM2_INTFR)
        *reg(address) &= value; /* W0C */
    else {
        *reg(address) = value;
        if (address == CH32V103_TIM2_SWEVGR && (value & 1u) != 0u) {
            *reg(CH32V103_TIM2_CNT) = 0u;
            if ((ch32v103_read16(CH32V103_TIM2_CTLR1) & (1u << 2)) == 0u)
                *reg(CH32V103_TIM2_INTFR) |= CH32V103_TIM_UIF;
        }
    }
}
uint32_t ch32v103_irq_lock(void)
{
    uint32_t previous = cpu_mstatus;
    cpu_mstatus &= ~CH32V103_MSTATUS_IRQ_ENABLE;
    return previous;
}
void ch32v103_irq_restore(uint32_t state) { cpu_mstatus = state; }
void ch32v103_cpu_irq_enable(void) { cpu_mstatus |= CH32V103_MSTATUS_IRQ_ENABLE; }
void ch32v103_cpu_irq_disable(void) { cpu_mstatus &= ~CH32V103_MSTATUS_IRQ_ENABLE; }
void ch32v103_install_mtvec(uintptr_t entry) { mtvec = entry; ++vector_installs; }
void ch32v103_io_fence(void) { ++fences; }

void ecos_ch32v103_exception_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval)
{
    assert(ecos_ch32v103_last_fault.mcause == mcause);
    assert(ecos_ch32v103_last_fault.mepc == mepc);
    assert(ecos_ch32v103_last_fault.mtval == mtval);
    assert((cpu_mstatus & CH32V103_MSTATUS_IRQ_ENABLE) == 0u);
    longjmp(fault_return, 1);
}

static void dispatch(uint32_t source)
{
    uint32_t state = ch32v103_irq_lock();
    ecos_ch32v103_trap_handler(0x80000000u | source, 0x1234u, 0u);
    ch32v103_irq_restore(state);
}
static int is_enabled(uint32_t source)
{
    return (enabled[source >> 5] & (1u << (source & 31u))) != 0u;
}
static void increment(void *arg) { ++*(unsigned *)arg; }

static void test_pfic(void)
{
    unsigned calls = 0u;
    static const uint32_t invalid[] = { 0u, 2u, 3u, 11u, 13u, 15u, 35u, 38u, 60u, UINT32_MAX };
    assert(ecos_interrupt_init() == ECOS_OK);
    assert(cpu_mstatus == 0x1888u);
    assert(mtvec == ((uintptr_t)ecos_ch32v103_vector_table | 1u));
    assert(ch32v103_read32(CH32V103_PFIC_CFGR) == 0xFA050003u);
    assert(enabled[0] == 0u && enabled[1] == 0u);
    assert(ch32v103_read32(CH32V103_PFIC_THRESHOLD) == 0u);
    assert(ecos_interrupt_init() == ECOS_OK && vector_installs == 1u);
    for (size_t index = 0u; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        assert(ecos_interrupt_alloc(invalid[index], 1u, increment, &calls) == ECOS_ERR_INVALID_ARGUMENT);
        assert(ecos_interrupt_enable(invalid[index]) == ECOS_ERR_INVALID_ARGUMENT);
        assert(ecos_interrupt_disable(invalid[index]) == ECOS_ERR_INVALID_ARGUMENT);
        assert(ecos_interrupt_free(invalid[index]) == ECOS_ERR_INVALID_ARGUMENT);
    }
    assert(ecos_interrupt_alloc(53u, 0u, increment, &calls) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_interrupt_alloc(53u, 17u, increment, &calls) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_interrupt_alloc(53u, 1u, NULL, &calls) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_interrupt_enable(53u) == ECOS_ERR_NOT_INITIALIZED);
    assert(ecos_interrupt_alloc(53u, 1u, increment, &calls) == ECOS_OK);
    assert(ch32v103_read8(CH32V103_PFIC_PRIORITY(53u)) == 0xF0u);
    assert(!is_enabled(53u));
    assert(ecos_interrupt_alloc(53u, 2u, increment, &calls) == ECOS_ERR_BUSY);
    assert(ecos_interrupt_enable(53u) == ECOS_OK && is_enabled(53u));
    dispatch(53u);
    assert(calls == 1u);
    ecos_interrupt_set_threshold(1u);
    assert(!is_enabled(53u));
    dispatch(53u);
    assert(calls == 1u);
    assert(ecos_interrupt_alloc(14u, 16u, increment, &calls) == ECOS_OK);
    assert(ch32v103_read8(CH32V103_PFIC_PRIORITY(14u)) == 0u);
    assert(ecos_interrupt_enable(14u) == ECOS_OK && is_enabled(14u));
    dispatch(14u);
    assert(calls == 2u);
    assert(ecos_interrupt_disable(14u) == ECOS_OK);
    ecos_interrupt_set_threshold(0u);
    assert(is_enabled(53u) && !is_enabled(14u));
    ecos_interrupt_set_threshold(UINT32_MAX);
    assert(!is_enabled(53u));
    ecos_interrupt_set_threshold(0u);
    assert(is_enabled(53u));
    pending[1] = 1u << (53u & 31u);
    assert(ecos_interrupt_free(53u) == ECOS_OK);
    assert(!is_enabled(53u) && pending[1] == 0u);
    assert(ecos_interrupt_enable(53u) == ECOS_ERR_NOT_INITIALIZED);
    assert(ecos_interrupt_free(14u) == ECOS_OK);
    assert(ecos_interrupt_free(14u) == ECOS_ERR_NOT_INITIALIZED);
    assert(ecos_interrupt_alloc(12u, 1u, increment, &calls) == ECOS_OK);
    assert(ecos_interrupt_free(12u) == ECOS_OK);
    /* Unallocated peripheral IRQ is masked rather than repeatedly dispatched. */
    enabled[1] |= 1u << (59u & 31u);
    dispatch(59u);
    assert(!is_enabled(59u));
    assert(cpu_mstatus == 0x1888u);
    ecos_interrupt_global_disable();
    assert((cpu_mstatus & 0x88u) == 0u);
    ecos_interrupt_global_enable();
    assert((cpu_mstatus & 0x88u) == 0x88u && vector_installs == 1u);
}

static unsigned timer_calls;
static void timer_handler(void *arg)
{
    assert(arg == &timer_calls);
    assert((ch32v103_read16(CH32V103_TIM2_INTFR) & 1u) == 0u);
    ++timer_calls;
}

static void test_timer(void)
{
    ecos_timer_config_t config = { 1000000u };
    uint32_t count = 0u;
    assert(ecos_timer_register_callback(0u, timer_handler, &timer_calls, 1u) == ECOS_ERR_NOT_INITIALIZED);
    assert(ecos_timer_init(0u, &config) == ECOS_OK);
    assert(ch32v103_read16(CH32V103_TIM2_PSC) == 127u);
    assert(ch32v103_read16(CH32V103_TIM2_ATRLR) == 62499u);
    assert(ch32v103_read16(CH32V103_TIM2_DMAINTENR) == 0u);
    *reg(CH32V103_TIM2_CNT) = 123u;
    assert(ecos_timer_get_count(0u, &count) == ECOS_OK && count == 1968u);
    assert(ecos_interrupt_alloc(44u, 2u, increment, &timer_calls) == ECOS_OK);
    assert(ecos_timer_register_callback(0u, timer_handler, &timer_calls, 1u) == ECOS_ERR_BUSY);
    assert(ch32v103_timer_irq_active == 0u);
    assert(ch32v103_read16(CH32V103_TIM2_DMAINTENR) == 0u);
    assert(ecos_interrupt_free(44u) == ECOS_OK);
    assert(ecos_timer_start(0u) == ECOS_OK);
    assert(ecos_timer_register_callback(0u, timer_handler, &timer_calls, 1u) == ECOS_ERR_BUSY);
    assert(ecos_timer_stop(0u) == ECOS_OK);
    assert(ecos_timer_register_callback(0u, timer_handler, &timer_calls, 1u) == ECOS_OK);
    assert(ch32v103_timer_irq_active == 1u && is_enabled(44u));
    assert(ch32v103_read16(CH32V103_TIM2_DMAINTENR) == 1u);
    assert(ecos_timer_register_callback(0u, timer_handler, &timer_calls, 1u) == ECOS_ERR_BUSY);
    assert(ecos_timer_init(0u, &config) == ECOS_ERR_BUSY);
    assert(ecos_timer_deinit(0u) == ECOS_ERR_BUSY);
    assert(ecos_timer_delay_us(0u, 10u) == ECOS_ERR_BUSY);
    assert(ecos_timer_start(0u) == ECOS_OK);
    assert(ch32v103_read16(CH32V103_TIM2_CNT) == 0u);
    assert((ch32v103_read16(CH32V103_TIM2_INTFR) & 1u) == 0u);
    dispatch(44u);
    assert(timer_calls == 0u);
    *reg(CH32V103_TIM2_INTFR) = 3u; /* UIF plus a capture flag */
    dispatch(44u);
    assert(timer_calls == 1u && ch32v103_read16(CH32V103_TIM2_INTFR) == 2u);
    assert(ecos_timer_stop(0u) == ECOS_OK);
    assert(ecos_timer_unregister_callback(0u) == ECOS_OK);
    assert(ch32v103_timer_irq_active == 0u && !is_enabled(44u));
    assert(ch32v103_read16(CH32V103_TIM2_DMAINTENR) == 0u);
    assert(ecos_timer_unregister_callback(0u) == ECOS_ERR_NOT_INITIALIZED);
    assert(ecos_timer_deinit(0u) == ECOS_OK);
    assert(ecos_timer_get_count(0u, &count) == ECOS_ERR_NOT_INITIALIZED);
    assert(ecos_timer_start(1u) == ECOS_ERR_INVALID_ARGUMENT);
    assert(hal_timer_clear_interrupt(0u) == ECOS_ERR_NOT_INITIALIZED);
    const uint32_t periods[] = { 1u, 10000u, 65536u, 65537u, 1000000u, 536870912u };
    for (size_t index = 0u; index < sizeof(periods) / sizeof(periods[0]); ++index) {
        config.period_us = periods[index];
        assert(ecos_timer_init(0u, &config) == ECOS_OK);
        uint32_t quantum = ((uint32_t)ch32v103_read16(CH32V103_TIM2_PSC) + 1u) / 8u;
        uint32_t actual = ((uint32_t)ch32v103_read16(CH32V103_TIM2_ATRLR) + 1u) * quantum;
        assert(actual >= periods[index] && actual - periods[index] < quantum);
        assert(ecos_timer_deinit(0u) == ECOS_OK);
    }
    config.period_us = 536870913u;
    assert(ecos_timer_init(0u, &config) == ECOS_ERR_UNSUPPORTED);
}

static unsigned exti_calls[16];
static int reedge;
static void exti_handler(void *arg)
{
    uint32_t pin = (uint32_t)(uintptr_t)arg;
    assert((ch32v103_read32(EXTI_INTFR) & (1u << pin)) == 0u || reedge != 0);
    ++exti_calls[pin];
    if (pin == 6u && reedge != 0)
        *reg(EXTI_INTFR) |= 1u << 8; /* new edge after dispatcher cleared its snapshot */
}

static void test_exti(void)
{
    assert(ecos_gpio_interrupt_set_trigger(ECOS_GPIO_PORT_0, 16u, ECOS_GPIO_INTERRUPT_ANYEDGE) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_gpio_interrupt_set_trigger(ECOS_GPIO_PORT_COUNT, 0u, ECOS_GPIO_INTERRUPT_ANYEDGE) == ECOS_ERR_INVALID_ARGUMENT);
    assert(ecos_gpio_interrupt_set_trigger(ECOS_GPIO_PORT_0, 0u, ECOS_GPIO_INTERRUPT_LOW_LEVEL) == ECOS_ERR_UNSUPPORTED);
    assert(ecos_gpio_interrupt_set_trigger(ECOS_GPIO_PORT_0, 0u, ECOS_GPIO_INTERRUPT_HIGH_LEVEL) == ECOS_ERR_UNSUPPORTED);
    assert(ecos_gpio_interrupt_enable(ECOS_GPIO_PORT_0, 0u) == ECOS_ERR_NOT_INITIALIZED);
    assert(ecos_gpio_interrupt_handler_add(ECOS_GPIO_PORT_0, 0u, exti_handler, NULL, 1u) == ECOS_ERR_NOT_INITIALIZED);
    assert(ecos_gpio_interrupt_set_trigger(ECOS_GPIO_PORT_0, 0u, ECOS_GPIO_INTERRUPT_ANYEDGE) == ECOS_OK);
    assert((ch32v103_read32(EXTI_RTENR) & 1u) != 0u && (ch32v103_read32(EXTI_FTENR) & 1u) != 0u);
    assert(ecos_gpio_interrupt_handler_add(ECOS_GPIO_PORT_0, 0u, exti_handler, NULL, 1u) == ECOS_OK);
    assert(ecos_gpio_interrupt_handler_add(ECOS_GPIO_PORT_1, 0u, exti_handler, NULL, 1u) == ECOS_ERR_BUSY);
    assert(ecos_gpio_interrupt_set_trigger(ECOS_GPIO_PORT_1, 0u, ECOS_GPIO_INTERRUPT_NEGEDGE) == ECOS_ERR_BUSY);
    assert(ecos_gpio_interrupt_enable(ECOS_GPIO_PORT_0, 0u) == ECOS_OK && is_enabled(22u));
    *reg(EXTI_INTFR) = 1u;
    dispatch(22u);
    assert(exti_calls[0] == 1u && ch32v103_read32(EXTI_INTFR) == 0u);
    assert(ecos_gpio_interrupt_handler_remove(ECOS_GPIO_PORT_0, 0u) == ECOS_OK && !is_enabled(22u));
    assert(ecos_gpio_interrupt_set_trigger(ECOS_GPIO_PORT_1, 0u, ECOS_GPIO_INTERRUPT_NEGEDGE) == ECOS_OK);
    assert((ch32v103_read32(AFIO_EXTICR(0u)) & 15u) == 1u);
    assert(ecos_gpio_interrupt_handler_add(ECOS_GPIO_PORT_1, 0u, exti_handler, NULL, 1u) == ECOS_OK);
    assert(ecos_gpio_interrupt_handler_remove(ECOS_GPIO_PORT_1, 0u) == ECOS_OK);

    /* EXTI6 / EXTI8 share source 39; preserve routes, handlers and enables. */
    *reg(AFIO_EXTICR(2u)) = 0xBA98u;
    assert(ecos_gpio_interrupt_set_trigger(ECOS_GPIO_PORT_1, 6u, ECOS_GPIO_INTERRUPT_POSEDGE) == ECOS_OK);
    assert(ecos_gpio_interrupt_set_trigger(ECOS_GPIO_PORT_2, 8u, ECOS_GPIO_INTERRUPT_NEGEDGE) == ECOS_OK);
    assert(ch32v103_read32(AFIO_EXTICR(2u)) == 0xBA92u);
    assert(ecos_interrupt_alloc(39u, 1u, increment, &timer_calls) == ECOS_OK);
    assert(ecos_gpio_interrupt_handler_add(ECOS_GPIO_PORT_1, 6u, exti_handler, (void *)(uintptr_t)6u, 2u) == ECOS_ERR_BUSY);
    assert(ecos_interrupt_free(39u) == ECOS_OK);
    assert(ecos_gpio_interrupt_handler_add(ECOS_GPIO_PORT_1, 6u, exti_handler, (void *)(uintptr_t)6u, 2u) == ECOS_OK);
    assert(ecos_gpio_interrupt_handler_add(ECOS_GPIO_PORT_2, 8u, exti_handler, (void *)(uintptr_t)8u, 1u) == ECOS_ERR_BUSY);
    assert(ecos_gpio_interrupt_handler_add(ECOS_GPIO_PORT_2, 8u, exti_handler, (void *)(uintptr_t)8u, 2u) == ECOS_OK);
    assert(ecos_gpio_interrupt_enable(ECOS_GPIO_PORT_1, 6u) == ECOS_OK);
    assert(ecos_gpio_interrupt_enable(ECOS_GPIO_PORT_2, 8u) == ECOS_OK);
    *reg(EXTI_INTFR) = (1u << 6) | (1u << 8) | (1u << 7);
    reedge = 1;
    dispatch(39u);
    reedge = 0;
    assert(exti_calls[6] == 1u && exti_calls[8] == 1u);
    assert(ch32v103_read32(EXTI_INTFR) == ((1u << 8) | (1u << 7)));
    dispatch(39u);
    assert(exti_calls[8] == 2u && ch32v103_read32(EXTI_INTFR) == (1u << 7));
    assert(ecos_gpio_interrupt_disable(ECOS_GPIO_PORT_1, 6u) == ECOS_OK && is_enabled(39u));
    assert(ecos_gpio_interrupt_handler_remove(ECOS_GPIO_PORT_1, 6u) == ECOS_OK && is_enabled(39u));
    assert(ecos_gpio_interrupt_handler_remove(ECOS_GPIO_PORT_2, 8u) == ECOS_OK && !is_enabled(39u));
    assert(ecos_interrupt_alloc(39u, 1u, increment, &timer_calls) == ECOS_OK);
    assert(ecos_interrupt_free(39u) == ECOS_OK);
    /* Disabling a trigger retains its registration but requires reconfiguration. */
    assert(ecos_gpio_interrupt_set_trigger(ECOS_GPIO_PORT_3, 15u, ECOS_GPIO_INTERRUPT_ANYEDGE) == ECOS_OK);
    assert(ecos_gpio_interrupt_handler_add(ECOS_GPIO_PORT_3, 15u, exti_handler, (void *)(uintptr_t)15u, 1u) == ECOS_OK);
    assert(ecos_gpio_interrupt_enable(ECOS_GPIO_PORT_3, 15u) == ECOS_OK && is_enabled(56u));
    assert(ecos_gpio_interrupt_set_trigger(ECOS_GPIO_PORT_3, 15u, ECOS_GPIO_INTERRUPT_DISABLE) == ECOS_OK);
    assert(ecos_gpio_interrupt_enable(ECOS_GPIO_PORT_3, 15u) == ECOS_ERR_NOT_INITIALIZED);
    assert(ecos_gpio_interrupt_handler_remove(ECOS_GPIO_PORT_3, 15u) == ECOS_OK);
    assert(cpu_mstatus == 0x1888u);
}

static void test_faults(void)
{
    if (setjmp(fault_return) == 0) {
        ecos_ch32v103_trap_handler(2u, 0x123456u, 0xDEADu);
        assert(0);
    }
    if (setjmp(fault_return) == 0) {
        ecos_ch32v103_trap_handler(0x80000023u, 0x100u, 0u); /* reserved IRQ35 */
        assert(0);
    }
}

int main(void)
{
    test_pfic();
    test_timer();
    test_exti();
    test_faults();
    assert(fences != 0u);
    puts("CH32V103 PFIC/TIM2/EXTI register tests passed");
    return 0;
}
