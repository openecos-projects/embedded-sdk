#include <stddef.h>
#include <stdint.h>

#include "hal_interrupt.h"
#include "board.h"

#define STARTYSKY_T1_PICO_MSTATUS_MIE             (1u << 3)
#define STARTYSKY_T1_PICO_MIE_MEIE                (1u << 11)
#define STARTYSKY_T1_PICO_MCAUSE_MACHINE_EXTERNAL 0x8000000Bu

struct startysky_t1_pico_intr_entry
{
    hal_intr_handler_t handler;
    void *arg;
};

extern void startysky_t1_pico_machine_trap_entry(void);

static struct startysky_t1_pico_intr_entry
    interrupt_entries[STARTYSKY_T1_PICO_PLIC_SOURCE_COUNT];
static uint8_t interrupt_initialized;

/**
 * 保存机器全局中断状态并临时关闭全局中断。
 */
static uint32_t startysky_t1_pico_intr_lock(void)
{
    uint32_t previous;

    /* 原子读取 mstatus 并清除机器全局中断使能位。 */
    __asm__ volatile("csrrc %0, mstatus, %1"
                     : "=r"(previous)
                     : "r"(STARTYSKY_T1_PICO_MSTATUS_MIE)
                     : "memory");
    return previous;
}


/**
 * 根据保存的机器状态恢复全局中断使能位。
 */
static void startysky_t1_pico_intr_unlock(uint32_t previous)
{
    /* 仅在进入临界区前已经开启时恢复机器全局中断。 */
    if ((previous & STARTYSKY_T1_PICO_MSTATUS_MIE) != 0u)
        __asm__ volatile("csrs mstatus, %0"
                         :
                         : "r"(STARTYSKY_T1_PICO_MSTATUS_MIE)
                         : "memory");
}


/**
 * 判断 PLIC 中断源编号是否位于可分配范围内。
 */
static uint8_t startysky_t1_pico_intr_source_is_valid(uint32_t source)
{
    /* source 0 保留为无中断，当前 enable 寄存器覆盖 source 1 至 31。 */
    return (uint8_t)((source != 0u) &&
                     (source < STARTYSKY_T1_PICO_PLIC_SOURCE_COUNT));
}


/**
 * 处理公共中断层未接管的同步异常或其他机器陷阱。
 */
__attribute__((weak, noreturn)) void hal_exception_handler(uint32_t mcause,
                                                            uint32_t mepc)
{
    /* 保留异常参数供调试器查看并关闭全部机器中断。 */
    (void)mcause;
    (void)mepc;
    hal_intr_global_disable();

    /* 停留在稳定循环中，避免跳过未知异常指令。 */
    for (;;)
        __asm__ volatile("nop");
}


/**
 * 初始化机器外部中断入口和当前机器模式 PLIC 上下文。
 */
int hal_intr_init(void)
{
    uintptr_t entry;
    uint32_t previous;

    /* 已经初始化时保持现有中断注册内容不变。 */
    if (interrupt_initialized != 0u)
        return 0;

    /* 在关闭全局中断的临界区内初始化 PLIC 和处理表。 */
    previous = startysky_t1_pico_intr_lock();
    REG_PLIC_ENABLE = 0u;
    REG_PLIC_THRESHOLD = 0u;
    for (uint32_t source = 0u;
         source < STARTYSKY_T1_PICO_PLIC_SOURCE_COUNT;
         ++source)
    {
        interrupt_entries[source].handler = NULL;
        interrupt_entries[source].arg = NULL;
    }

    /* 安装四字节对齐的机器模式直接陷阱入口。 */
    entry = (uintptr_t)startysky_t1_pico_machine_trap_entry;
    __asm__ volatile("csrw mtvec, %0"
                     :
                     : "r"(entry & ~(uintptr_t)0x3u)
                     : "memory");
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    interrupt_initialized = 1u;
    startysky_t1_pico_intr_unlock(previous);
    return 0;
}


/**
 * 为指定 PLIC 中断源分配一个静态处理函数入口。
 */
int hal_intr_alloc(uint32_t source,
                   uint32_t priority,
                   hal_intr_handler_t handler,
                   void *arg)
{
    uint32_t previous;

    /* 拒绝无效 source、零优先级和空处理函数。 */
    if ((startysky_t1_pico_intr_source_is_valid(source) == 0u) ||
        (priority == 0u) || (handler == NULL))
        return -1;

    /* 确保机器陷阱入口在首次分配前已经安装。 */
    if (hal_intr_init() != 0)
        return -1;

    /* 在临界区内拒绝重复分配并写入静态处理表。 */
    previous = startysky_t1_pico_intr_lock();
    if (interrupt_entries[source].handler != NULL)
    {
        startysky_t1_pico_intr_unlock(previous);
        return -1;
    }

    interrupt_entries[source].arg = arg;
    interrupt_entries[source].handler = handler;
    REG_PLIC_PRIORITY(source) = priority;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    startysky_t1_pico_intr_unlock(previous);
    return 0;
}


/**
 * 释放指定 PLIC 中断源及其静态处理函数。
 */
int hal_intr_free(uint32_t source)
{
    uint32_t previous;

    /* 拒绝超出当前 PLIC 实现范围的中断源。 */
    if (startysky_t1_pico_intr_source_is_valid(source) == 0u)
        return -1;

    /* 在临界区内禁止 source 并清除处理表和优先级。 */
    previous = startysky_t1_pico_intr_lock();
    REG_PLIC_ENABLE &= ~(1u << source);
    REG_PLIC_PRIORITY(source) = 0u;
    interrupt_entries[source].handler = NULL;
    interrupt_entries[source].arg = NULL;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    startysky_t1_pico_intr_unlock(previous);
    return 0;
}


/**
 * 使能已经分配处理函数的指定 PLIC 中断源。
 */
int hal_intr_enable(uint32_t source)
{
    uint32_t previous;

    /* 只允许使能已经注册处理函数的有效中断源。 */
    if ((startysky_t1_pico_intr_source_is_valid(source) == 0u) ||
        (interrupt_entries[source].handler == NULL))
        return -1;

    /* 原子设置当前机器模式上下文的 source 使能位。 */
    previous = startysky_t1_pico_intr_lock();
    REG_PLIC_ENABLE |= 1u << source;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    startysky_t1_pico_intr_unlock(previous);
    return 0;
}


/**
 * 禁止指定 PLIC 中断源。
 */
int hal_intr_disable(uint32_t source)
{
    uint32_t previous;

    /* 拒绝超出当前 PLIC 实现范围的中断源。 */
    if (startysky_t1_pico_intr_source_is_valid(source) == 0u)
        return -1;

    /* 原子清除当前机器模式上下文的 source 使能位。 */
    previous = startysky_t1_pico_intr_lock();
    REG_PLIC_ENABLE &= ~(1u << source);
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    startysky_t1_pico_intr_unlock(previous);
    return 0;
}


/**
 * 设置当前机器模式 PLIC 上下文的优先级阈值。
 */
void hal_intr_set_threshold(uint32_t threshold)
{
    /* 写入阈值并确保配置先于后续中断使能生效。 */
    REG_PLIC_THRESHOLD = threshold;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
}


/**
 * 开启机器外部中断和机器全局中断。
 */
void hal_intr_global_enable(void)
{
    /* 先开启机器外部中断，再开启机器全局中断。 */
    __asm__ volatile("csrs mie, %0"
                     :
                     : "r"(STARTYSKY_T1_PICO_MIE_MEIE)
                     : "memory");
    __asm__ volatile("csrs mstatus, %0"
                     :
                     : "r"(STARTYSKY_T1_PICO_MSTATUS_MIE)
                     : "memory");
}


/**
 * 关闭机器全局中断和机器外部中断。
 */
void hal_intr_global_disable(void)
{
    /* 先关闭机器全局中断，再关闭机器外部中断。 */
    __asm__ volatile("csrc mstatus, %0"
                     :
                     : "r"(STARTYSKY_T1_PICO_MSTATUS_MIE)
                     : "memory");
    __asm__ volatile("csrc mie, %0"
                     :
                     : "r"(STARTYSKY_T1_PICO_MIE_MEIE)
                     : "memory");
}


/**
 * 分发机器模式陷阱并完成当前 PLIC 中断源。
 */
void startysky_t1_pico_machine_trap_handler(uint32_t mcause, uint32_t mepc)
{
    hal_intr_handler_t handler;
    void *arg;
    uint32_t source;

    /* 非机器外部中断交给可覆盖的异常处理函数。 */
    if (mcause != STARTYSKY_T1_PICO_MCAUSE_MACHINE_EXTERNAL)
        hal_exception_handler(mcause, mepc);

    /* 声明当前最高优先级中断源，无待处理中断时直接返回。 */
    source = REG_PLIC_CLAIM;
    if (startysky_t1_pico_intr_source_is_valid(source) == 0u)
        return;

    /* 获取稳定的处理函数和参数快照。 */
    handler = interrupt_entries[source].handler;
    arg = interrupt_entries[source].arg;

    /* 未注册 source 被立即禁止，避免持续进入空处理路径。 */
    if (handler == NULL)
        REG_PLIC_ENABLE &= ~(1u << source);
    else
        handler(arg);

    /* 完成外设处理后向 PLIC 写回本次 source。 */
    REG_PLIC_COMPLETE = source;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
}
