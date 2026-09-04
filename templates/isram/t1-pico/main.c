#include "main.h"

#define ISRAM_WORD_BYTES 4u

/**
 * 使用固定数据填充已验证的 ISRAM 区域。
 */
static void isram_fill(uint32_t pattern)
{
    uintptr_t address;
    uintptr_t end = STARTYSKY_T1_PICO_ISRAM_BASE_ADDR +
                    STARTYSKY_T1_PICO_ISRAM_TEST_SIZE_BYTES;

    /* 逐字写入整个十六千字节测试范围。 */
    for (address = STARTYSKY_T1_PICO_ISRAM_BASE_ADDR;
         address < end;
         address += ISRAM_WORD_BYTES)
        *(volatile uint32_t *)address = pattern ^ (uint32_t)address;

    __asm__ volatile("fence rw, rw" : : : "memory");
}


/**
 * 校验已验证 ISRAM 区域中的固定地址模式。
 */
static int isram_verify(uint32_t pattern)
{
    uintptr_t address;
    uintptr_t end = STARTYSKY_T1_PICO_ISRAM_BASE_ADDR +
                    STARTYSKY_T1_PICO_ISRAM_TEST_SIZE_BYTES;

    /* 逐字比较整个测试范围中的地址异或数据。 */
    for (address = STARTYSKY_T1_PICO_ISRAM_BASE_ADDR;
         address < end;
         address += ISRAM_WORD_BYTES)
    {
        if (*(volatile uint32_t *)address !=
            (pattern ^ (uint32_t)address))
            return -1;
    }

    return 0;
}


/**
 * 执行十六千字节 ISRAM 固定数据和地址模式测试。
 */
int main(void)
{
    /* 初始化串口并测试两组互补地址模式。 */
    hal_sys_uart_init();
    hal_sys_putstr("StartySky T1-Pico ISRAM 16 KiB test started.\n");
    isram_fill(0xA5A5A5A5u);
    if (isram_verify(0xA5A5A5A5u) != 0)
        hal_sys_putstr("ISRAM TEST FAIL: pattern A5\n");
    else
    {
        isram_fill(0x5A5A5A5Au);
        if (isram_verify(0x5A5A5A5Au) != 0)
            hal_sys_putstr("ISRAM TEST FAIL: pattern 5A\n");
        else
            hal_sys_putstr("ISRAM 16 KiB TEST PASS\n");
    }

    /* 保留最终测试结果供串口和调试器观察。 */
    for (;;)
        __asm__ volatile("nop");
}
