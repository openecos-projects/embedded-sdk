# RTC 驱动

自由运行计数器加闹钟比较（不是日历钟）。

头文件：`drivers/rtc/include/ecos/driver/rtc.h`。
示例：`example/peripherals/rtc/counter`。

## 类型与常量

```c
typedef enum {
    ECOS_RTC_ID_0 = 0,   /* 唯一实例 */
    ECOS_RTC_ID_COUNT
} ecos_rtc_id_t;

#define ECOS_RTC_DEFAULT ((ecos_rtc_id_t)ECOS_RTC_ID_0)

typedef struct {
    uint32_t prescaler;      /* 输入时钟分频比，1 表示不分频 */
    uint8_t alarm_enabled;   /* 非零使能闹钟触发 */
} ecos_rtc_config_t;

#define ECOS_RTC_CONFIG_DEFAULT { 1u, 0u }
```

## 函数

```c
ecos_err_t ecos_rtc_init(ecos_rtc_id_t id, const ecos_rtc_config_t *config);
ecos_err_t ecos_rtc_deinit(ecos_rtc_id_t id);
ecos_err_t ecos_rtc_get_counter(ecos_rtc_id_t id, uint32_t *counter);
ecos_err_t ecos_rtc_set_counter(ecos_rtc_id_t id, uint32_t counter);
ecos_err_t ecos_rtc_set_alarm(ecos_rtc_id_t id, uint32_t alarm);
int ecos_rtc_alarm_triggered(ecos_rtc_id_t id);
```

- `ecos_rtc_init()` — 配置分频比并开始计数；`alarm_enabled` 非零时同时
  使能闹钟触发。
- `ecos_rtc_get_counter()` / `ecos_rtc_set_counter()` — 读写计数器。
- `ecos_rtc_set_alarm()` — 设置闹钟比较值（计数值到达时触发）。
- `ecos_rtc_alarm_triggered()` — 触发返回 1，未触发返回 0，失败返回负
  错误码。

## 最小使用骨架

```c
#include "ecos/bsp/console.h"
#include "ecos/driver/rtc.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#define LOG_TAG "rtc"

int main(void)
{
    const ecos_rtc_config_t config = { 50000u, 0u };  /* 50 MHz 输入时 1 kHz */
    uint32_t before;
    uint32_t after;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_rtc_init(ECOS_RTC_DEFAULT, &config),
                        "initialize RTC");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_rtc_get_counter(ECOS_RTC_DEFAULT, &before),
                        "read counter");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 100u),
                        "delay");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_rtc_get_counter(ECOS_RTC_DEFAULT, &after),
                        "re-read counter");
    (void)ECOS_LOGI(LOG_TAG, "counter: %u -> %u", before, after);
    for (;;) { }
}
```

## 注意事项

- 该 IP 是计数器加闹钟比较，**不是日历钟**；日历功能请用外置 RTC 器件
  （如 PCF8563，见 [devices.md](devices.md)）。
- 计数频率 = 输入时钟 / `prescaler`；输入时钟源由 SoC 定义。
- 闹钟状态用 `ecos_rtc_alarm_triggered()` 轮询，没有中断回调 API。
- 目前仅 ysyx-2512-1 与 ysyx-2512-2 提供该 IP；CL1-2512（T1-Pico）没有。
- 不要与 2.x 遗留头文件 `hal/rtc/hal_rtc.h` 混用。
