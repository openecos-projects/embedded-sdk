#include "main.h"

#define GPIO_TEST_PORT     0u
#define GPIO_TEST_PIN      0u
#define GPIO_TEST_PRIORITY 3u

static volatile uint32_t gpio_interrupt_count;

/**
 * 记录一次 GPIOA0 外部中断。
 */
static void gpio_interrupt_handler(void *arg)
{
    /* 中断上下文只更新计数，串口输出留给主循环。 */
    (void)arg;
    ++gpio_interrupt_count;
}


/**
 * 输出失败信息并停止测试。
 */
__attribute__((noreturn)) static void gpio_interrupt_fail(const char *stage)
{
    /* 关闭全局中断并输出失败阶段。 */
    hal_intr_global_disable();
    hal_sys_putstr("GPIO INTERRUPT TEST FAIL: ");
    hal_sys_putstr((char *)stage);
    hal_sys_putstr("\n");

    for (;;)
        __asm__ volatile("nop");
}


/**
 * 持续验证 GPIOA0 下降沿外部中断 callback。
 */
int main(void)
{
    uint32_t reported_count = 0u;

    /* 初始化串口、中断入口和 GPIOA0 输入模式。 */
    hal_sys_uart_init();
    hal_sys_putstr("StartySky T1 GPIOA0 falling-edge interrupt test.\n");
    hal_sys_putstr("Drive GPIOA0 HIGH then LOW to trigger.\n");
    if (hal_intr_init() != 0)
        gpio_interrupt_fail("interrupt init");

    gpio_hal_input_enable(GPIO_TEST_PORT, GPIO_TEST_PIN);
    if (gpio_hal_set_intr_type(GPIO_TEST_PORT,
                               GPIO_TEST_PIN,
                               GPIO_INTR_NEGEDGE) != 0)
        gpio_interrupt_fail("trigger config");

    /* 注册单引脚 callback 并依次开启外设、PLIC 和 CPU 中断。 */
    if (gpio_hal_isr_handler_add(GPIO_TEST_PORT,
                                 GPIO_TEST_PIN,
                                 gpio_interrupt_handler,
                                 (void *)0,
                                 GPIO_TEST_PRIORITY) != 0)
        gpio_interrupt_fail("handler add");

    if (gpio_hal_intr_enable(GPIO_TEST_PORT, GPIO_TEST_PIN) != 0)
        gpio_interrupt_fail("interrupt enable");

    hal_intr_set_threshold(0u);
    hal_intr_global_enable();

    /* 在主循环中为每次成功回调输出确认信息。 */
    for (;;)
    {
        if (reported_count != gpio_interrupt_count)
        {
            reported_count = gpio_interrupt_count;
            hal_sys_putstr("GPIOA0 INTERRUPT PASS\n");
        }

        __asm__ volatile("nop");
    }
}
