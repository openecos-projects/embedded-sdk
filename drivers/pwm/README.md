# PWM 驱动

`driver-pwm` 是 SDK 3.0 面向应用的公共 PWM 接口。应用包含
`ecos/driver/pwm.h`，不直接访问 HAL 或 SoC 寄存器。

- `ecos_pwm_init()` 配置 PWM 控制器的时钟分频和周期，但不启动输出；
- `ecos_pwm_set_duty_cycle()` 配置指定通道 `0..100%` 的占空比；
- `ecos_pwm_start()` 启动已经配置的 PWM 控制器；
- `ecos_pwm_stop()` 停止 PWM 控制器并保留当前配置；
- `ecos_pwm_get_instance_count()` 返回当前 Target 提供的控制器数量。

当前 ysyx-2512 Target 提供 PWM0 和 PWM1，每个控制器有四个通道。StarrySky L4
上的通道引脚为：

| 控制器 | 通道 | 引脚 |
| --- | --- | --- |
| PWM0 | 0..3 | GPIO1[14..17] |
| PWM1 | 0..3 | GPIO1[18..21] |

Driver 的 `clock_divider` 表示 PWM 输入时钟的整数分频系数，`period_ticks` 表示一个
PWM 周期包含的分频后时钟 tick 数。输出频率为
`PWM 输入时钟 / clock_divider / period_ticks`。
