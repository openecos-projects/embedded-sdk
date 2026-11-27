# PWM 驱动

脉宽调制输出：配置周期与分频，按通道设置占空比。

头文件：`drivers/pwm/include/ecos/driver/pwm.h`。
示例包括 `pwm-basic`、`pwm-buzzer` 和 [四路舵机 PWM](../../examples/peripherals/pwm/servo-four/README.md)。

## 类型与常量

```c
typedef uint8_t ecos_pwm_id_t;               /* PWM 控制器实例号 */

#define ECOS_PWM_DEFAULT ((ecos_pwm_id_t)0u)  /* 默认实例 0 */

typedef enum {
    ECOS_PWM_CHANNEL_0 = 0,
    ECOS_PWM_CHANNEL_1,
    ECOS_PWM_CHANNEL_2,
    ECOS_PWM_CHANNEL_3,
    ECOS_PWM_CHANNEL_COUNT
} ecos_pwm_channel_t;                        /* 控制器内的输出通道 */

typedef struct {
    uint32_t clock_divider;  /* 输入时钟分频 */
    uint32_t period_ticks;   /* 周期长度（计数 tick 数） */
} ecos_pwm_config_t;

#define ECOS_PWM_CONFIG_DEFAULT { 1u, 1000u }
```

输出频率 = 控制器输入时钟 / `clock_divider` / `period_ticks`。

## 函数

```c
int ecos_pwm_get_instance_count(void);
ecos_err_t ecos_pwm_init(ecos_pwm_id_t pwm, const ecos_pwm_config_t *config);
ecos_err_t ecos_pwm_set_duty_cycle(ecos_pwm_id_t pwm, ecos_pwm_channel_t channel,
                                   uint8_t duty_percent);
ecos_err_t ecos_pwm_set_pulse_ticks(ecos_pwm_id_t pwm, ecos_pwm_channel_t channel,
                                    uint32_t high_ticks);
ecos_err_t ecos_pwm_start(ecos_pwm_id_t pwm);
ecos_err_t ecos_pwm_stop(ecos_pwm_id_t pwm);
```

- `ecos_pwm_get_instance_count()` — 返回当前 Target 的 PWM 控制器数量。
- `ecos_pwm_init()` — 配置控制器，**不启动计数器**。
- `ecos_pwm_set_duty_cycle()` — 设置某通道占空比，范围 0..100（百分比，含
  端点）；运行中也可更新。
- `ecos_pwm_set_pulse_ticks()` 设置指定通道每周期的高电平 tick 数，范围为
  `0..period_ticks`（含端点）。0 表示常低，等于周期表示常高。每 tick 的时长为
  `clock_divider / PWM 输入时钟频率` 秒。实例尚未初始化返回
  `ECOS_ERR_NOT_INITIALIZED`，超出当前周期或选用无效实例、通道返回
  `ECOS_ERR_INVALID_ARGUMENT`。
- `ecos_pwm_start()` / `ecos_pwm_stop()` — 启动/停止整个控制器的计数。

## 最小使用骨架

```c
#include "ecos/bsp/console.h"
#include "ecos/driver/pwm.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#define LOG_TAG "pwm"

int main(void)
{
    const ecos_pwm_config_t config = {
        .clock_divider = 50u,
        .period_ticks = 1000u,
    };

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_pwm_init(ECOS_PWM_DEFAULT, &config),
                        "configure PWM");

    for (;;) {
        ECOS_PANIC_ON_ERROR(LOG_TAG,
                            ecos_pwm_set_duty_cycle(ECOS_PWM_DEFAULT,
                                                    ECOS_PWM_CHANNEL_0, 25u),
                            "set duty 25%");
        ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_pwm_start(ECOS_PWM_DEFAULT), "start");
        ECOS_PANIC_ON_ERROR(LOG_TAG,
                            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 2000u),
                            "delay");
        ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_pwm_stop(ECOS_PWM_DEFAULT), "stop");
        ECOS_PANIC_ON_ERROR(LOG_TAG,
                            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 2000u),
                            "delay");
    }
}
```

## 注意事项

- `init` 只写配置不启动输出，别忘了 `start`；`stop` 后再次 `start` 沿用上次
  占空比。
- 占空比按整个控制器的所有通道分别设置，但周期/分频是控制器级的，同控制器
  的各通道共享频率。
- CH32V103 的普通比较值在下一个周期边界生效。周期为 65536 ticks 时，进入或退出
  100% 使用通道强制输出模式，切换可以立即影响该路当前脉冲。其他通道继续计数。
- 蜂鸣器的音调换算见 [buzzer 驱动](devices.md)。
