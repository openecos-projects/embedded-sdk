#include "main.h"

/**
 * 比较两个 CLINT 高低位数值是否相同。
 */
static int clint_value_equal(const hal_clint_value_t *left,
                             const hal_clint_value_t *right)
{
    /* 同时比较高三十二位和低三十二位。 */
    return (left->high == right->high) && (left->low == right->low);
}


/**
 * 输出失败信息并停止测试。
 */
__attribute__((noreturn)) static void clint_test_fail(const char *stage)
{
    /* 输出失败阶段并保留程序现场。 */
    hal_sys_putstr("CLINT TEST FAIL: ");
    hal_sys_putstr((char *)stage);
    hal_sys_putstr("\n");

    for (;;)
        __asm__ volatile("nop");
}


/**
 * 验证 CLINT mtime 和 mtimecmp 高低位读写接口。
 */
int main(void)
{
    const hal_clint_value_t test_time = {0x11112222u, 0x33334444u};
    const hal_clint_value_t test_compare = {0xAAAABBBBu, 0xCCCCDDDDu};
    hal_clint_value_t original_time;
    hal_clint_value_t original_compare;
    hal_clint_value_t actual;

    /* 初始化串口并保存测试前的计数和比较寄存器。 */
    hal_sys_uart_init();
    hal_sys_putstr("StartySky T1-Pico CLINT register test started.\n");
    if ((hal_clint_get_mtime(&original_time) != 0) ||
        (hal_clint_get_mtimecmp(&original_compare) != 0))
        clint_test_fail("initial read");

    /* 写入并回读 mtimecmp 测试值。 */
    if ((hal_clint_set_mtimecmp(&test_compare) != 0) ||
        (hal_clint_get_mtimecmp(&actual) != 0) ||
        !clint_value_equal(&actual, &test_compare))
        clint_test_fail("mtimecmp readback");

    /* 写入并读取 mtime，允许低位在读取前自然递增。 */
    if ((hal_clint_set_mtime(&test_time) != 0) ||
        (hal_clint_get_mtime(&actual) != 0) ||
        (actual.high != test_time.high) ||
        (actual.low < test_time.low))
        clint_test_fail("mtime readback");

    /* 恢复测试前的 mtime 和 mtimecmp 内容。 */
    if ((hal_clint_set_mtime(&original_time) != 0) ||
        (hal_clint_set_mtimecmp(&original_compare) != 0))
        clint_test_fail("restore");

    hal_sys_putstr("CLINT REGISTER TEST PASS\n");
    for (;;)
        __asm__ volatile("nop");
}
