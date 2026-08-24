#include <am.h>
#include <amdev.h>
#include <stdint.h>

#include "hal_sys_uart.h"
#include "hal_timer.h"

static uint32_t timer_last;
static uint64_t timer_high;


/**
 * 初始化 L4 AM 系统计时器。
 */
void __am_timer_init(void)
{
    /* 启动 Timer0 的微秒计数并清除软件回绕状态。 */
    hal_sys_tick_init(0u);
    timer_last = hal_get_sys_tick(0u);
    timer_high = 0u;
}


/**
 * 返回 L4 Timer0 的 AM 计时配置。
 */
static void __am_timer_config(AM_TIMER_CONFIG_T *cfg)
{
    /* L4 当前只提供 uptime，没有经过验证的 RTC。 */
    cfg->present = true;
    cfg->has_rtc = false;
}


/**
 * 返回当前的 AM uptime。
 */
static void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime)
{
    uint32_t now;

    /* 读取 32 位 HAL 计数并检测回绕。 */
    now = hal_get_sys_tick(0u);
    if (now < timer_last)
        timer_high += (1ull << 32);
    timer_last = now;

    /* 合并软件维护的高位和硬件低位。 */
    uptime->us = timer_high | now;
}


/**
 * 返回安全的空 RTC 值。
 */
static void __am_timer_rtc(AM_TIMER_RTC_T *rtc)
{
    /* L4 未声明 RTC 能力，所有字段返回确定的零值。 */
    rtc->second = 0;
    rtc->minute = 0;
    rtc->hour = 0;
    rtc->day = 0;
    rtc->month = 0;
    rtc->year = 0;
}


/**
 * 返回 UART 设备能力。
 */
static void __am_uart_config(AM_UART_CONFIG_T *cfg)
{
    /* SYS UART 支持发送和非阻塞接收。 */
    cfg->present = true;
}


/**
 * 通过 HAL 发送 AM UART 字符。
 */
static void __am_uart_tx(AM_UART_TX_T *tx)
{
    /* 复用 TRM 的 HAL 输出路径。 */
    hal_sys_putchar(tx->data);
}


/**
 * 读取 AM UART 字符。
 */
static void __am_uart_rx(AM_UART_RX_T *rx)
{
    /* 无输入时由 getch 返回 -1。 */
    rx->data = getch();
}


/**
 * 返回输入设备能力。
 */
static void __am_input_config(AM_INPUT_CONFIG_T *cfg)
{
    /* L4 当前没有已验证的 PS2 后端。 */
    cfg->present = false;
}


/**
 * 返回空键盘事件。
 */
static void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd)
{
    /* 不访问不存在的 PS2 MMIO，返回无按键状态。 */
    kbd->keydown = false;
    kbd->keycode = AM_KEY_NONE;
}


/**
 * 返回图形设备能力。
 */
static void __am_gpu_config(AM_GPU_CONFIG_T *cfg)
{
    /* L4 首版未验证显示设备，不宣称 GPU 支持。 */
    cfg->present = false;
    cfg->has_accel = false;
    cfg->width = 0;
    cfg->height = 0;
    cfg->vmemsz = 0;
}


/**
 * 返回 GPU 就绪状态。
 */
static void __am_gpu_status(AM_GPU_STATUS_T *status)
{
    /* 不存在的设备保持安全的未就绪状态。 */
    status->ready = false;
}


/**
 * 返回音频设备能力。
 */
static void __am_audio_config(AM_AUDIO_CONFIG_T *cfg)
{
    /* L4 没有完整 Audio HAL。 */
    cfg->present = false;
    cfg->bufsize = 0;
}


/**
 * 返回磁盘设备能力。
 */
static void __am_disk_config(AM_DISK_CONFIG_T *cfg)
{
    /* L4 当前没有块设备 HAL。 */
    cfg->present = false;
    cfg->blksz = 0;
    cfg->blkcnt = 0;
}


/**
 * 返回网络设备能力。
 */
static void __am_net_config(AM_NET_CONFIG_T *cfg)
{
    /* L4 当前没有网络 HAL。 */
    cfg->present = false;
}


typedef void (*handler_t)(void *buf);

static void *lut[32] = {
    [AM_UART_CONFIG] = (handler_t)__am_uart_config,
    [AM_UART_TX] = (handler_t)__am_uart_tx,
    [AM_UART_RX] = (handler_t)__am_uart_rx,
    [AM_TIMER_CONFIG] = (handler_t)__am_timer_config,
    [AM_TIMER_RTC] = (handler_t)__am_timer_rtc,
    [AM_TIMER_UPTIME] = (handler_t)__am_timer_uptime,
    [AM_INPUT_CONFIG] = (handler_t)__am_input_config,
    [AM_INPUT_KEYBRD] = (handler_t)__am_input_keybrd,
    [AM_GPU_CONFIG] = (handler_t)__am_gpu_config,
    [AM_GPU_STATUS] = (handler_t)__am_gpu_status,
    [AM_AUDIO_CONFIG] = (handler_t)__am_audio_config,
    [AM_DISK_CONFIG] = (handler_t)__am_disk_config,
    [AM_NET_CONFIG] = (handler_t)__am_net_config,
};


/**
 * 初始化 L4 IOE 设备分发表。
 */
bool ioe_init(void)
{
    /* 在程序启用 IOE 时启动 Timer0 系统时基。 */
    __am_timer_init();
    return true;
}


/**
 * 读取 AM IOE 寄存器。
 */
void ioe_read(int reg, void *buf)
{
    /* 未登记寄存器返回安全零值，不触碰旧平台地址。 */
    if (reg < 0 || reg >= (int)(sizeof(lut) / sizeof(lut[0])) || lut[reg] == 0)
    {
        *(uint32_t *)buf = 0u;
        return;
    }

    ((handler_t)lut[reg])(buf);
}


/**
 * 写入 AM IOE 寄存器。
 */
void ioe_write(int reg, void *buf)
{
    /* 发送类寄存器复用同一分发表，其余写操作安全忽略。 */
    if (reg == AM_UART_TX && reg < (int)(sizeof(lut) / sizeof(lut[0])))
        ((handler_t)lut[reg])(buf);
}
