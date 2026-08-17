#include <stddef.h>
#include <stdint.h>

#include "hal_timer.h"
#include "board.h"

/* 保存系统滴答初始化时的 CLINT 低位计数。 */
static uint32_t system_tick_origin;

/* 标记 Timer0 是否已经被周期中断 callback 占用。 */
uint8_t startysky_t1_timer_interrupt_active;

/**
 * 停止 Timer0 并清除已经锁存的完成状态。
 */
static void startysky_t1_timer_stop_and_clear(void)
{
    uint32_t eoi_value;

    /* 停止 Timer0，避免清理期间开始新的计数周期。 */
    REG_TIMER_0_CONTROL = 0u;
    __asm__ volatile("fence iorw, iorw" : : : "memory");

    /* 读取 EOI 寄存器以清除已经锁存的完成状态。 */
    eoi_value = REG_TIMER_0_EOI;
    (void)eoi_value;
}


/**
 * 使用 Timer0 轮询等待指定数量的定时器时钟。
 */
static uint8_t startysky_t1_timer_wait_ticks(uint32_t ticks)
{
    uint32_t poll_count = 0u;
    uint32_t eoi_value;

    /* 配置前停止 Timer0 并清除历史完成状态。 */
    startysky_t1_timer_stop_and_clear();

    /* 写入本次轮询延时使用的计数初值。 */
    REG_TIMER_0_LOAD_COUNT = ticks;

    /* 选择用户定义周期并保持完成状态非屏蔽。 */
    REG_TIMER_0_CONTROL = STARTYSKY_T1_TIMER_CONTROL_USER;
    __asm__ volatile("fence iorw, iorw" : : : "memory");

    /* 设置使能位并开始本次计数。 */
    REG_TIMER_0_CONTROL = STARTYSKY_T1_TIMER_CONTROL_USER |
                          STARTYSKY_T1_TIMER_CONTROL_ENABLE;
    __asm__ volatile("fence iorw, iorw" : : : "memory");

    /* 在与装载计数相同的软件轮询上限内等待完成状态。 */
    while ((REG_TIMER_0_INT_STATUS == 0u) && (poll_count < ticks))
        poll_count += 1u;

    /* 定时器未在上限内完成时恢复关闭状态并报告失败。 */
    if (REG_TIMER_0_INT_STATUS == 0u)
    {
        startysky_t1_timer_stop_and_clear();
        return 1u;
    }

    /* 读取 EOI 清除完成状态，然后停止 Timer0。 */
    eoi_value = REG_TIMER_0_EOI;
    (void)eoi_value;
    REG_TIMER_0_CONTROL = 0u;
    return 0u;
}


/**
 * 按指定时间单位将较长延时拆分为 Timer0 可装载的计数块。
 */
static uint8_t startysky_t1_timer_delay_scaled(uint8_t timer_id,
                                               uint32_t value,
                                               uint32_t ticks_per_unit)
{
    uint32_t maximum_units;

    /* StartySky T1 当前只提供 Timer0 给通用 Timer HAL 使用。 */
    if ((timer_id != 0u) || (ticks_per_unit == 0u) ||
        (startysky_t1_timer_interrupt_active != 0u))
        return 1u;

    /* 零长度延时直接成功返回。 */
    if (value == 0u)
        return 0u;

    /* 计算单次装载不会发生三十二位溢出的最大单位数。 */
    maximum_units = UINT32_MAX / ticks_per_unit;

    /* 将完整延时拆分为一个或多个硬件轮询周期。 */
    while (value != 0u)
    {
        uint32_t current_units = value;

        /* 将当前分块限制在单次可安全装载的范围内。 */
        if (current_units > maximum_units)
            current_units = maximum_units;

        /* 等待当前分块对应的硬件时钟数。 */
        if (startysky_t1_timer_wait_ticks(current_units * ticks_per_unit) != 0u)
            return 1u;

        /* 扣除已经完成的时间单位并继续处理剩余延时。 */
        value -= current_units;
    }

    /* 报告全部轮询延时已经完成。 */
    return 0u;
}


/**
 * 使用 Timer0 轮询延时指定微秒数。
 */
uint8_t hal_delay_us(uint8_t timer_id, uint32_t val)
{
    /* 按每微秒二十个 Timer0 时钟执行轮询延时。 */
    return startysky_t1_timer_delay_scaled(timer_id,
                                           val,
                                           STARTYSKY_T1_TIMER_CLOCK_HZ / 1000000u);
}


/**
 * 使用 Timer0 轮询延时指定毫秒数。
 */
uint8_t hal_delay_ms(uint8_t timer_id, uint32_t val)
{
    /* 按每毫秒二万个 Timer0 时钟执行轮询延时。 */
    return startysky_t1_timer_delay_scaled(timer_id,
                                           val,
                                           STARTYSKY_T1_TIMER_CLOCK_HZ / 1000u);
}


/**
 * 使用 Timer0 轮询延时指定秒数。
 */
uint8_t hal_delay_s(uint8_t timer_id, uint32_t val)
{
    /* 按每秒二千万个 Timer0 时钟执行轮询延时。 */
    return startysky_t1_timer_delay_scaled(timer_id,
                                           val,
                                           STARTYSKY_T1_TIMER_CLOCK_HZ);
}


/**
 * 使用 CLINT 低位计数初始化毫秒系统滴答基准。
 */
uint8_t hal_sys_tick_init(uint8_t timer_id)
{
    /* StartySky T1 当前只接受 Timer0 对应的系统滴答编号。 */
    if (timer_id != 0u)
        return 1u;

    /* 保存当前 CLINT 低位计数作为系统滴答起点。 */
    system_tick_origin = REG_CLINT_MTIME_LOW;
    return 0u;
}


/**
 * 返回系统滴答初始化后经过的毫秒数。
 */
uint32_t hal_get_sys_tick(uint8_t timer_id)
{
    uint32_t elapsed_ticks;

    /* 对无效的 Timer 编号返回零。 */
    if (timer_id != 0u)
        return 0u;

    /* 使用无符号减法保留 CLINT 低位计数的自然回绕语义。 */
    elapsed_ticks = REG_CLINT_MTIME_LOW - system_tick_origin;

    /* 将五兆赫兹 CLINT 计数转换为毫秒。 */
    return elapsed_ticks / (STARTYSKY_T1_CLINT_MTIME_FREQ_HZ / 1000u);
}


/**
 * 使用指定周期计数初始化 Timer0。
 */
int hal_timer_init(uint8_t timer_id, const hal_timer_config_t *config)
{
    /* 当前仅支持 Timer0，并拒绝空配置或零周期。 */
    if ((timer_id != 0u) || (config == NULL) ||
        (startysky_t1_timer_interrupt_active != 0u) ||
        (config->period_ticks == 0u))
        return -1;

    /* 停止定时器、清除历史中断并写入周期装载值。 */
    startysky_t1_timer_stop_and_clear();
    REG_TIMER_0_LOAD_COUNT = config->period_ticks;

    /* 默认使用周期模式并屏蔽中断，注册 callback 后再解除屏蔽。 */
    REG_TIMER_0_CONTROL = STARTYSKY_T1_TIMER_CONTROL_USER |
                          STARTYSKY_T1_TIMER_CONTROL_MASK;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return 0;
}


/**
 * 关闭 Timer0 并清除其周期配置。
 */
int hal_timer_deinit(uint8_t timer_id)
{
    /* 当前仅支持 Timer0。 */
    if ((timer_id != 0u) ||
        (startysky_t1_timer_interrupt_active != 0u))
        return -1;

    /* 停止并清除中断状态后清零装载寄存器。 */
    startysky_t1_timer_stop_and_clear();
    REG_TIMER_0_LOAD_COUNT = 0u;
    return 0;
}


/**
 * 启动已经初始化的 Timer0。
 */
int hal_timer_start(uint8_t timer_id)
{
    /* 当前仅支持 Timer0，且零装载值不允许启动。 */
    if ((timer_id != 0u) || (REG_TIMER_0_LOAD_COUNT == 0u))
        return -1;

    /* 保留周期和中断屏蔽配置并设置使能位。 */
    REG_TIMER_0_CONTROL |= STARTYSKY_T1_TIMER_CONTROL_ENABLE;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return 0;
}


/**
 * 停止 Timer0 并保留其周期和中断配置。
 */
int hal_timer_stop(uint8_t timer_id)
{
    /* 当前仅支持 Timer0。 */
    if (timer_id != 0u)
        return -1;

    /* 清除使能位并等待寄存器访问完成。 */
    REG_TIMER_0_CONTROL &= ~STARTYSKY_T1_TIMER_CONTROL_ENABLE;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return 0;
}


/**
 * 读取 Timer0 当前剩余计数值。
 */
int hal_timer_get_count(uint8_t timer_id, uint32_t *count)
{
    /* 拒绝无效 Timer 编号或空输出指针。 */
    if ((timer_id != 0u) || (count == NULL))
        return -1;

    /* 返回当前计数寄存器值。 */
    *count = REG_TIMER_0_CURRENT_VALUE;
    return 0;
}


/**
 * 读取 Timer0 EOI 寄存器以清除锁存中断。
 */
int hal_timer_clear_interrupt(uint8_t timer_id)
{
    uint32_t eoi_value;

    /* 当前仅支持 Timer0。 */
    if (timer_id != 0u)
        return -1;

    /* EOI 寄存器采用读清除语义。 */
    eoi_value = REG_TIMER_0_EOI;
    (void)eoi_value;
    return 0;
}
