#include "main.h"

#define TIMER_TEST_ID       0u
#define TIMER_TEST_TICKS    20000000u
#define TIMER_TEST_PRIORITY 3u

static volatile uint32_t timer_interrupt_count;

/**
 * 记录一次 Timer0 周期中断。
 */
static void timer_interrupt_handler(void *arg)
{
    /* 中断上下文只更新计数，避免在 callback 中轮询串口。 */
    (void)arg;
    ++timer_interrupt_count;
}


/**
 * 输出失败信息并停止测试。
 */
__attribute__((noreturn)) static void timer_interrupt_fail(const char *stage)
{
    /* 关闭全局中断和 Timer0 后输出失败阶段。 */
    hal_intr_global_disable();
    (void)hal_timer_stop(TIMER_TEST_ID);
    hal_sys_putstr("TIMER INTERRUPT TEST FAIL: ");
    hal_sys_putstr((char *)stage);
    hal_sys_putstr("\n");

    for (;;)
        __asm__ volatile("nop");
}


/**
 * 持续验证 Timer0 每秒产生一次 PLIC 外部中断。
 */
int main(void)
{
    const hal_timer_config_t config = {TIMER_TEST_TICKS};
    uint32_t reported_count = 0u;

    /* 初始化串口、机器中断入口和 Timer0 周期。 */
    hal_sys_uart_init();
    hal_sys_putstr("StartySky T1 Timer0 interrupt test started.\n");
    if (hal_intr_init() != 0)
        timer_interrupt_fail("interrupt init");

    if (hal_timer_init(TIMER_TEST_ID, &config) != 0)
        timer_interrupt_fail("timer init");

    /* 注册 Timer0 callback 并开启 PLIC、CPU 和定时器。 */
    if (hal_timer_register_callback(TIMER_TEST_ID,
                                    timer_interrupt_handler,
                                    (void *)0,
                                    TIMER_TEST_PRIORITY) != 0)
        timer_interrupt_fail("handler add");

    hal_intr_set_threshold(0u);
    hal_intr_global_enable();
    if (hal_timer_start(TIMER_TEST_ID) != 0)
        timer_interrupt_fail("timer start");

    /* 在主循环中为每次成功周期中断输出一行信息。 */
    for (;;)
    {
        if (reported_count != timer_interrupt_count)
        {
            reported_count = timer_interrupt_count;
            hal_sys_putstr("TIMER0 1-SECOND INTERRUPT PASS\n");
        }

        __asm__ volatile("nop");
    }
}
