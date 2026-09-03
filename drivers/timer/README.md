# Timer 定时器驱动

`driver-timer` 是 SDK 3.0 面向应用的公共定时器接口。应用包含
`ecos/driver/timer.h`，不直接包含内部 HAL 头文件或访问 SoC 寄存器。

## 跨板卡实例模型

公共接口不假定所有 Target 都提供相同数量的定时器。只需要一路定时器的应用使用
`ECOS_TIMER_DEFAULT`；需要使用更多实例时，必须先调用
`ecos_timer_get_instance_count()` 查询当前 Target 实际提供的数量，然后在
`[0, instance_count)` 范围内选择实例。

通用 Example 只能依赖默认实例。遍历全部实例属于 Target 专属硬件验证，不应放入要求
跨板卡工作的 Example。

## 时间单位与控制接口

Timer Driver 使用微秒作为周期配置单位；支持读取计数值的 Target 也以微秒为单位：

- `ecos_timer_init()`：配置周期，但不启动计数。
- `ecos_timer_start()`：从零重新启动已配置的周期。
- `ecos_timer_stop()`：停止计数并清除到期状态，保留周期配置。
- `ecos_timer_get_count()`：读取当前原始计数值；不支持读取的 Target 返回
  `ECOS_ERR_UNSUPPORTED`。
- `ecos_timer_is_expired()`：查询当前周期是否到期。
- `ecos_timer_deinit()`：停止定时器并释放配置。
- `ecos_timer_delay_us/ms/s()`：使用指定实例执行阻塞式轮询延时。

阻塞式延时会临时替换所选实例的已有配置，并在结束前停止和释放该实例。毫秒和秒延时
不依赖硬件乘除法，适用于 RV32E 等没有 M 扩展的目标。

## ysyx-2512 实现

ysyx-2512 提供 Timer0 至 Timer3，共四个实例。L4 默认使用 25 MHz Timer 输入时钟，
HAL 将其预分频为 1 MHz，因此一个计数对应一微秒。

该 Timer IP 未接出 CNT 读值，因此 ysyx-2512 的 `ecos_timer_get_count()` 返回
`ECOS_ERR_UNSUPPORTED`；轮询延时仅通过 STAT 到期标志判断结果。

当前只支持基础控制和轮询模式。Timer 中断需要先完成 ysyx-2512 中断控制器适配，
在此之前公共 Driver 不提供 callback 接口。

## 延时示例

```c
#include "ecos/driver/timer.h"

static int wait_500_ms(void)
{
    return ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 500u);
}
```
