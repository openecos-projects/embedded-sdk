#include <stdint.h>

#include "hal_sdram.h"
#include "board.h"

#define STARTYSKY_T1_SDRAM_COMMAND_NOP       0x0000000Bu
#define STARTYSKY_T1_SDRAM_COMMAND_PRECHARGE 0x00000009u
#define STARTYSKY_T1_SDRAM_COMMAND_MODE       0x0000000Au
#define STARTYSKY_T1_SDRAM_REFRESH_INITIAL   0x0000000Au
#define STARTYSKY_T1_SDRAM_REFRESH_NORMAL    0x00000320u
#define STARTYSKY_T1_SDRAM_CONTROL_NORMAL    0x01F8000Au

/**
 * 执行 SDRAM 初始化命令之间的已验证软件延时。
 */
static void startysky_t1_sdram_delay(uint32_t count)
{
    volatile uint32_t remaining = count;

    /* 使用易变计数确保优化构建仍保留完整等待循环。 */
    while (remaining != 0u)
    {
        --remaining;
        __asm__ volatile("nop");
    }
}


/**
 * 初始化 StartySky T1 的三十二兆字节外部 SDRAM。
 */
int hal_sdram_init(void)
{
    uint32_t readback;

    /* 发送两次空操作命令并等待 SDRAM 启动时序。 */
    REG_SDRAM_COMMAND = STARTYSKY_T1_SDRAM_COMMAND_NOP;
    readback = REG_SDRAM_COMMAND;
    (void)readback;
    startysky_t1_sdram_delay(100u);
    REG_SDRAM_COMMAND = STARTYSKY_T1_SDRAM_COMMAND_NOP;
    readback = REG_SDRAM_COMMAND;
    (void)readback;
    startysky_t1_sdram_delay(200u);

    /* 预充电全部存储体并执行初始刷新等待。 */
    REG_SDRAM_COMMAND = STARTYSKY_T1_SDRAM_COMMAND_PRECHARGE;
    readback = REG_SDRAM_COMMAND;
    (void)readback;
    REG_SDRAM_REFRESH = STARTYSKY_T1_SDRAM_REFRESH_INITIAL;
    readback = REG_SDRAM_REFRESH;
    (void)readback;
    startysky_t1_sdram_delay(9u);

    /* 设置正常刷新周期并发送模式寄存器命令。 */
    REG_SDRAM_REFRESH = STARTYSKY_T1_SDRAM_REFRESH_NORMAL;
    readback = REG_SDRAM_REFRESH;
    (void)readback;
    REG_SDRAM_COMMAND = STARTYSKY_T1_SDRAM_COMMAND_MODE;
    readback = REG_SDRAM_COMMAND;
    (void)readback;
    __asm__ volatile("fence rw, rw" : : : "memory");

    /* 通过已验证地址读取向 SDRAM 提供模式寄存器位。 */
    readback = REG_SDRAM_MODE_ACCESS;
    (void)readback;
    __asm__ volatile("fence rw, rw" : : : "memory");

    /* 配置十六位数据总线并进入正常工作模式。 */
    REG_SDRAM_CONTROL = STARTYSKY_T1_SDRAM_CONTROL_NORMAL;
    readback = REG_SDRAM_CONTROL;
    REG_SDRAM_COMMAND = 0u;
    (void)REG_SDRAM_COMMAND;
    __asm__ volatile("fence rw, rw" : : : "memory");

    /* 通过控制寄存器回读结果报告初始化状态。 */
    return readback == STARTYSKY_T1_SDRAM_CONTROL_NORMAL ? 0 : -1;
}
