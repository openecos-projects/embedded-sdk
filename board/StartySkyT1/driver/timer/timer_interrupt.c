#include <stddef.h>
#include <stdint.h>

#include "hal_interrupt.h"
#include "hal_timer.h"
#include "board.h"

static hal_timer_callback_t timer0_callback;
static void *timer0_callback_arg;

extern uint8_t startysky_t1_timer_interrupt_active;

/**
 * 清除 Timer0 中断并调用用户处理函数。
 */
static void startysky_t1_timer0_intr_dispatch(void *arg)
{
    hal_timer_callback_t callback = timer0_callback;

    /* 明确忽略公共中断层传入的固定参数。 */
    (void)arg;

    /* 在调用用户处理函数前读取 EOI 清除 Timer0 中断。 */
    (void)hal_timer_clear_interrupt(0u);
    __asm__ volatile("fence iorw, iorw" : : : "memory");

    /* 使用稳定快照调用当前 Timer0 用户处理函数。 */
    if (callback != NULL)
        callback(timer0_callback_arg);
}


/**
 * 为 Timer0 注册周期中断处理函数并开启 PLIC source。
 */
int hal_timer_register_callback(uint8_t timer_id,
                                hal_timer_callback_t callback,
                                void *arg,
                                uint32_t priority)
{
    /* 当前仅支持 Timer0，且每个定时器只允许一个处理函数。 */
    if ((timer_id != 0u) || (callback == NULL) ||
        (timer0_callback != NULL) || (REG_TIMER_0_LOAD_COUNT == 0u))
        return -1;

    /* 保存用户处理信息并分配 Timer0 的 PLIC source 3。 */
    timer0_callback_arg = arg;
    timer0_callback = callback;
    if (hal_intr_alloc(STARTYSKY_T1_TIMER0_PLIC_SOURCE,
                       priority,
                       startysky_t1_timer0_intr_dispatch,
                       NULL) != 0)
    {
        timer0_callback = NULL;
        timer0_callback_arg = NULL;
        return -1;
    }

    /* 解除 Timer0 外设中断屏蔽并开启对应 PLIC source。 */
    REG_TIMER_0_CONTROL &= ~STARTYSKY_T1_TIMER_CONTROL_MASK;
    if (hal_intr_enable(STARTYSKY_T1_TIMER0_PLIC_SOURCE) != 0)
    {
        REG_TIMER_0_CONTROL |= STARTYSKY_T1_TIMER_CONTROL_MASK;
        (void)hal_intr_free(STARTYSKY_T1_TIMER0_PLIC_SOURCE);
        timer0_callback = NULL;
        timer0_callback_arg = NULL;
        return -1;
    }

    /* 标记 Timer0 已进入中断模式，阻止轮询延时重写配置。 */
    startysky_t1_timer_interrupt_active = 1u;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return 0;
}


/**
 * 删除 Timer0 周期中断处理函数并恢复外设屏蔽。
 */
int hal_timer_unregister_callback(uint8_t timer_id)
{
    /* 当前仅支持已经注册处理函数的 Timer0。 */
    if ((timer_id != 0u) || (timer0_callback == NULL))
        return -1;

    /* 先屏蔽 Timer0 外设中断并禁止 PLIC source。 */
    REG_TIMER_0_CONTROL |= STARTYSKY_T1_TIMER_CONTROL_MASK;
    (void)hal_intr_disable(STARTYSKY_T1_TIMER0_PLIC_SOURCE);
    (void)hal_timer_clear_interrupt(0u);

    /* 释放 PLIC source 并清除用户处理信息。 */
    if (hal_intr_free(STARTYSKY_T1_TIMER0_PLIC_SOURCE) != 0)
        return -1;

    /* 释放 callback 后允许轮询延时重新使用 Timer0。 */
    startysky_t1_timer_interrupt_active = 0u;
    timer0_callback = NULL;
    timer0_callback_arg = NULL;
    return 0;
}
