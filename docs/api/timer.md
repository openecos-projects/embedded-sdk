# Timer 驱动

周期定时、到期查询与阻塞延时。

头文件：`drivers/timer/include/ecos/driver/timer.h`。
示例：`example/get_start/blink`（`ecos_timer_delay_ms` 是示例程序的标准延时
手段）。

## 类型与常量

```c
typedef uint8_t ecos_timer_id_t;               /* Timer 实例号 */

#define ECOS_TIMER_DEFAULT ((ecos_timer_id_t)0u) /* 默认实例 0 */

typedef struct {
    uint32_t period_us;   /* 周期（微秒） */
} ecos_timer_config_t;

#define ECOS_TIMER_CONFIG_DEFAULT { 1000u }   /* 1 ms 周期 */
```

## 函数

```c
int ecos_timer_get_instance_count(void);
ecos_err_t ecos_timer_init(ecos_timer_id_t timer, const ecos_timer_config_t *config);
ecos_err_t ecos_timer_deinit(ecos_timer_id_t timer);
ecos_err_t ecos_timer_start(ecos_timer_id_t timer);
ecos_err_t ecos_timer_stop(ecos_timer_id_t timer);
ecos_err_t ecos_timer_get_count(ecos_timer_id_t timer, uint32_t *count);
int ecos_timer_is_expired(ecos_timer_id_t timer);
ecos_err_t ecos_timer_delay_us(ecos_timer_id_t timer, uint32_t duration_us);
ecos_err_t ecos_timer_delay_ms(ecos_timer_id_t timer, uint32_t duration_ms);
ecos_err_t ecos_timer_delay_s(ecos_timer_id_t timer, uint32_t duration_s);
```

- `ecos_timer_get_instance_count()` — 返回当前 Target 提供的 Timer 实例数量。
- `ecos_timer_init()` / `ecos_timer_deinit()` — 初始化/反初始化。
- `ecos_timer_start()` — 启动并**从零重新开始**已配置的周期；
  `ecos_timer_stop()` 停止。
- `ecos_timer_get_count()` — 读原始计数值；未接出计数值的 Target 返回
  `ECOS_ERR_UNSUPPORTED`。
- `ecos_timer_is_expired()` — 到期返回 1，未到期返回 0，失败返回负错误码。
- `ecos_timer_delay_us/ms/s()` — 阻塞轮询延时；**会临时替换所选实例的配置**，
  延时结束后恢复。

## 最小使用骨架

```c
#include "ecos/bsp/console.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#define LOG_TAG "timer"

int main(void)
{
    const ecos_timer_config_t config = ECOS_TIMER_CONFIG_DEFAULT;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_timer_init(ECOS_TIMER_DEFAULT, &config),
                        "initialize timer");
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_timer_start(ECOS_TIMER_DEFAULT), "start");

    for (;;) {
        int expired = ecos_timer_is_expired(ECOS_TIMER_DEFAULT);

        ECOS_PANIC_ON_ERROR(LOG_TAG, expired, "query timer");
        if (expired == 1) {
            (void)ECOS_LOGI(LOG_TAG, "period elapsed");
            ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_timer_start(ECOS_TIMER_DEFAULT),
                                "restart period");
        }
    }
}
```

如果只是要延时，无需 init/start，直接
`ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 500u)` 即可（见 blink 示例）。

## 注意事项

- 轮询延时函数（`delay_us/ms/s`）会在内部临时改写该 Timer 实例的配置，因此
  **不要与同一实例的周期计时（start/is_expired）混用**；需要两者时请用不同
  实例。
- `get_count` 在部分 Target 上返回 `ECOS_ERR_UNSUPPORTED`，移植代码时做好
  分支处理。
- 没有中断回调 API，定时事件用 `is_expired` 轮询。
- 不要与 2.x 遗留头文件 `hal/timer/hal_timer.h` 混用：其 `hal_timer_config_t`
  与新契约 `ecos/hal/timer.h` 中的同名类型冲突（见 [README.md](README.md) 的
  遗留警告）。
