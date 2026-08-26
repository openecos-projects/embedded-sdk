#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#include "generated/autoconf.h"
#include "hal_qspi.h"
#include "hal_sys_uart.h"

extern char _heap_start;
extern char _heap_end;

int main(const char *args);

Area heap = RANGE(&_heap_start, &_heap_end);

static const char mainargs[MAINARGS_MAX_LEN] = MAINARGS_PLACEHOLDER;


/**
 * 向 L4 系统串口发送一个字符。
 */
void putch(char ch)
{
    /* 统一通过 SDK UART HAL 输出，避免访问旧 SoC 地址。 */
    hal_sys_putchar(ch);
}


/**
 * 轮询读取一个 L4 系统串口字符。
 */
char getch(void)
{
    uint8_t data = 0xffu;

    /* 无数据时返回 AM 约定的无效字符。 */
    if (hal_sys_try_getchar(&data) == 0u)
        return (char)-1;

    return (char)data;
}


/**
 * 停止当前单核程序。
 */
void halt(int code)
{
    /* base profile 不使用 EBREAK，保持核心可预测地停在安全循环。 */
    (void)code;
    while (1)
        ;
}


/**
 * 从 L4 Flash 读取一个 32 位字。
 */
uint32_t flash_read(uint32_t addr)
{
    static bool initialized;
    uint8_t data[4] = {0u, 0u, 0u, 0u};

    /* 首次读取时初始化 Flash 使用的 QSPI 控制器。 */
    if (!initialized)
    {
        if (hal_qspi_init(HAL_QSPI_PORT_0,
                          &(hal_qspi_config_t){.clkdiv = 3u}) != 0)
            return 0u;
        initialized = true;
    }

    /* 通过 QSPI HAL 发送标准 Read Data 命令。 */
    if (hal_qspi_read(HAL_QSPI_PORT_0, 0x03u, 8u, addr, 24u, 0u,
                      data, sizeof(data)) != 0)
        return 0u;

    /* 按 Flash 字节序组装 AM 需要的返回值。 */
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) |
           (uint32_t)data[3];
}


/**
 * 初始化 AM 运行时并调用外部 kernel 的 main 函数。
 */
void _trm_init(void)
{
    /* 最小 TRM 只初始化所有白名单程序都需要的 UART。 */
    hal_sys_uart_init();

    /* 进入 am-kernels 程序并在返回后停止。 */
    halt(main(mainargs));
}
