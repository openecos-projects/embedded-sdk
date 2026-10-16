#include "cl1_2512_soc.h"
#include "ecos/heap.h"

#include <stdint.h>

/* T1-Pico SDRAM 初始化序列，已在目标硬件上验证。 */
#define SDRAM_COMMAND_NOP        0x0000000Bu
#define SDRAM_COMMAND_PRECHARGE  0x00000009u
#define SDRAM_COMMAND_MODE       0x0000000Au
#define SDRAM_REFRESH_INITIAL    0x0000000Au
#define SDRAM_REFRESH_NORMAL     0x00000320u
#define SDRAM_CONTROL_NORMAL     0x01F8000Au

extern uint8_t _heap_start[];
extern uint8_t _heap_end[];

static void sdram_delay(uint32_t count)
{
    volatile uint32_t remaining = count;

    while (remaining != 0u) {
        --remaining;
        __asm__ volatile("nop");
    }
}

static void sdram_controller_init(void)
{
    /* 两次空操作命令并等待启动时序。 */
    REG_SDRAM_COMMAND = SDRAM_COMMAND_NOP;
    (void)REG_SDRAM_COMMAND;
    sdram_delay(100u);
    REG_SDRAM_COMMAND = SDRAM_COMMAND_NOP;
    (void)REG_SDRAM_COMMAND;
    sdram_delay(200u);

    /* 预充电全部存储体并执行初始刷新等待。 */
    REG_SDRAM_COMMAND = SDRAM_COMMAND_PRECHARGE;
    (void)REG_SDRAM_COMMAND;
    REG_SDRAM_REFRESH = SDRAM_REFRESH_INITIAL;
    (void)REG_SDRAM_REFRESH;
    sdram_delay(9u);

    /* 设置正常刷新周期并发送模式寄存器命令。 */
    REG_SDRAM_REFRESH = SDRAM_REFRESH_NORMAL;
    (void)REG_SDRAM_REFRESH;
    REG_SDRAM_COMMAND = SDRAM_COMMAND_MODE;
    (void)REG_SDRAM_COMMAND;
    __asm__ volatile("fence rw, rw" : : : "memory");

    /* 通过已验证地址读取向 SDRAM 提供模式寄存器位。 */
    (void)REG_SDRAM_MODE_ACCESS;
    __asm__ volatile("fence rw, rw" : : : "memory");

    /* 配置十六位数据总线并进入正常工作模式。 */
    REG_SDRAM_CONTROL = SDRAM_CONTROL_NORMAL;
    (void)REG_SDRAM_CONTROL;
    REG_SDRAM_COMMAND = 0u;
    (void)REG_SDRAM_COMMAND;
    __asm__ volatile("fence rw, rw" : : : "memory");
}

/* 由 start.S 在 C 运行环境就绪后、main 之前调用：
 * 初始化 SDRAM 控制器，并向 heap 组件注册 SRAM/SDRAM 两个内存池。 */
void ecos_soc_early_init(void)
{
    sdram_controller_init();

    (void)ecos_heap_add_pool((uintptr_t)_heap_start,
                             (size_t)(_heap_end - _heap_start),
                             ECOS_HEAP_ZONE_INTERNAL);
    (void)ecos_heap_add_pool(CL1_2512_SDRAM_BASE_ADDR,
                             (size_t)CL1_2512_SDRAM_SIZE_BYTES,
                             ECOS_HEAP_ZONE_EXTERNAL);
}
