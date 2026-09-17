# RTC 计数器驱动

`driver-rtc` 是 SDK 3.0 面向应用的 RTC 接口。应用包含
`ecos/driver/rtc.h`，不直接包含内部 HAL 头文件或访问 SoC 寄存器。

## 接口

- `ecos_rtc_init()`：配置输入时钟分频比并使能计数；`alarm_enabled` 非零时
  同时使能闹钟触发。
- `ecos_rtc_get_counter()` / `ecos_rtc_set_counter()`：读写自由运行计数器。
- `ecos_rtc_set_alarm()`：设置闹钟比较值。
- `ecos_rtc_alarm_triggered()`：轮询闹钟是否触发。
- `ecos_rtc_deinit()`：停止计数。

`ECOS_RTC_DEFAULT` 选择唯一实例。该 IP 是计数器加闹钟比较，**不是日历钟**；
日历功能请使用外置 RTC 器件（如 PCF8563，见 devices）。

## ysyx-2512 实现

ysyx-2512-1 与 ysyx-2512-2 均提供该 IP（寄存器起始于 0x10004000）。计数频率为
输入时钟经 `prescaler` 分频；输入时钟源由 SoC 定义。

```c
#include "ecos/driver/rtc.h"

const ecos_rtc_config_t config = { 50000u, 0u };   /* 50 MHz 输入时 1 kHz 计数 */
uint32_t counter;

if (ecos_rtc_init(ECOS_RTC_DEFAULT, &config) == ECOS_OK &&
    ecos_rtc_get_counter(ECOS_RTC_DEFAULT, &counter) == ECOS_OK) {
    /* ... */
}
```
