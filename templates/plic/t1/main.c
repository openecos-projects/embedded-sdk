#include "main.h"

#define PLIC_TEST_SOURCE   3u
#define PLIC_TEST_PRIORITY 3u

/**
 * 提供 PLIC 分配状态测试使用的空处理函数。
 */
static void plic_test_handler(void *arg)
{
    /* 本模板只验证注册状态机，不主动产生硬件中断。 */
    (void)arg;
}


/**
 * 输出失败信息并停止测试。
 */
__attribute__((noreturn)) static void plic_test_fail(const char *stage)
{
    /* 输出失败阶段并保留程序现场。 */
    hal_sys_putstr("PLIC TEST FAIL: ");
    hal_sys_putstr((char *)stage);
    hal_sys_putstr("\n");

    for (;;)
        __asm__ volatile("nop");
}


/**
 * 验证 PLIC 中断源分配、启停和释放状态机。
 */
int main(void)
{
    /* 初始化串口和机器模式中断基础设施。 */
    hal_sys_uart_init();
    hal_sys_putstr("StartySky T1 PLIC API test started.\n");
    if (hal_intr_init() != 0)
        plic_test_fail("init");

    /* 验证无效 source 和有效 source 的分配结果。 */
    if (hal_intr_alloc(0u,
                       PLIC_TEST_PRIORITY,
                       plic_test_handler,
                       (void *)0) == 0)
        plic_test_fail("invalid source accepted");

    if (hal_intr_alloc(PLIC_TEST_SOURCE,
                       PLIC_TEST_PRIORITY,
                       plic_test_handler,
                       (void *)0) != 0)
        plic_test_fail("alloc");

    /* 验证同一 source 不允许重复分配。 */
    if (hal_intr_alloc(PLIC_TEST_SOURCE,
                       PLIC_TEST_PRIORITY,
                       plic_test_handler,
                       (void *)0) == 0)
        plic_test_fail("duplicate alloc accepted");

    /* 验证 source 使能、禁止和释放接口。 */
    hal_intr_set_threshold(0u);
    if (hal_intr_enable(PLIC_TEST_SOURCE) != 0)
        plic_test_fail("enable");

    if (hal_intr_disable(PLIC_TEST_SOURCE) != 0)
        plic_test_fail("disable");

    if (hal_intr_free(PLIC_TEST_SOURCE) != 0)
        plic_test_fail("free");

    /* 释放后不得再次使能未注册 source。 */
    if (hal_intr_enable(PLIC_TEST_SOURCE) == 0)
        plic_test_fail("enable after free accepted");

    hal_sys_putstr("PLIC API TEST PASS\n");
    for (;;)
        __asm__ volatile("nop");
}
