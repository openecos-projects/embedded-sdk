#include "main.h"

#define SDRAM_WORD_BYTES 4u

/**
 * 使用固定值和地址生成 SDRAM 测试数据。
 */
static uint32_t sdram_pattern(uintptr_t address, uint32_t seed)
{
    /* 地址异或固定种子可以覆盖数据线和地址线变化。 */
    return (uint32_t)address ^ seed;
}


/**
 * 向全部三十二兆字节 SDRAM 写入地址模式。
 */
static void sdram_fill(uint32_t seed)
{
    uintptr_t address;

    /* 逐字写入整个 SDRAM 地址范围。 */
    for (address = STARTYSKY_T1_PICO_SDRAM_BASE_ADDR;
         address < STARTYSKY_T1_PICO_SDRAM_END_ADDR;
         address += SDRAM_WORD_BYTES)
        *(volatile uint32_t *)address = sdram_pattern(address, seed);

    __asm__ volatile("fence rw, rw" : : : "memory");
}


/**
 * 校验全部三十二兆字节 SDRAM 地址模式。
 */
static int sdram_verify(uint32_t seed)
{
    uintptr_t address;

    /* 逐字回读并在首次数据不匹配时报告失败。 */
    for (address = STARTYSKY_T1_PICO_SDRAM_BASE_ADDR;
         address < STARTYSKY_T1_PICO_SDRAM_END_ADDR;
         address += SDRAM_WORD_BYTES)
    {
        if (*(volatile uint32_t *)address != sdram_pattern(address, seed))
            return -1;
    }

    return 0;
}


/**
 * 在二十兆赫兹基线下初始化并扫描全部 SDRAM。
 */
int main(void)
{
    /* 初始化串口和 SDRAM，不执行任何 PLL 配置或时钟切换。 */
    hal_sys_uart_init();
    hal_sys_putstr("StartySky T1-Pico SDRAM 32 MiB test at 20 MHz started.\n");
    if (hal_sdram_init() != 0)
        hal_sys_putstr("SDRAM TEST FAIL: initialization\n");
    else
    {
        /* 执行两组互补地址模式的全范围写入和回读。 */
        sdram_fill(0xA5A5A5A5u);
        if (sdram_verify(0xA5A5A5A5u) != 0)
            hal_sys_putstr("SDRAM TEST FAIL: pattern A5\n");
        else
        {
            sdram_fill(0x5A5A5A5Au);
            if (sdram_verify(0x5A5A5A5Au) != 0)
                hal_sys_putstr("SDRAM TEST FAIL: pattern 5A\n");
            else
                hal_sys_putstr("SDRAM 32 MiB TEST PASS\n");
        }
    }

    /* 保留最终测试结果供串口和调试器观察。 */
    for (;;)
        __asm__ volatile("nop");
}
