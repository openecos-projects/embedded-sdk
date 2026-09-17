# ECOS SDK 3.x 公共 API 概览

本文是 SDK 3.x 应用层公共 API 的总入口，说明分层架构、统一的错误模型、配置与
命名惯例，并给出标准应用骨架。各模块的详细参考见子页面：

- [bsp.md](bsp.md) — BSP：控制台、LED、按键、板级资源
- [core.md](core.md) — 核心运行时：错误码与日志
- [gpio.md](gpio.md) / [uart.md](uart.md) / [i2c.md](i2c.md) / [pwm.md](pwm.md) /
  [timer.md](timer.md) / [qspi.md](qspi.md) — 总线与外设驱动
- [archinfo.md](archinfo.md) / [rng.md](rng.md) / [crc.md](crc.md) /
  [rcu.md](rcu.md) / [rtc.md](rtc.md) — 芯片服务类驱动（标识、随机数、校验、
  时钟复位、计数闹钟）
- [devices.md](devices.md) — 设备驱动（AHT20、AT24C64、蜂鸣器、ESP01S、SGP30、
  PCF8563、ST7735、ST7789、TM1650）
- [components.md](components.md) — 应用向组件（libc 子集、LightCoroutine、sfud、
  fatfs、letter-shell）

## 分层架构

```
Application（用户应用）
    ↓ 只能调用以下公共层
BSP（ecos/bsp/*）/ Device Driver（ecos/device/*）/ Component（应用向组件）
    ↓
Driver（ecos/driver/*，gpio / uart / i2c / pwm / timer / qspi / archinfo / rng /
crc / rcu / rtc）
    ↓
HAL（ecos/hal/*，SDK 内部接口，不承诺稳定）
    ↓
SoC / LL（寄存器与引脚复用，由 Target/Board 决定）
```

**应用只能使用以下公共 API：**

- 外设驱动：`drivers/` 发布的 `ecos/driver/*.h`
- 板级支持包：`board/include/ecos/bsp/*.h` 与生成的 `ecos/board_resources.h`
- 设备驱动：`devices/` 发布的 `ecos/device/*.h`
- 应用向组件：`components/` 中的 libc 子集、LightCoroutine、sfud、fatfs、
  letter-shell（见 [components.md](components.md)）
- 核心运行时：`ecos/error.h`、`ecos/log.h`

**应用不得直接使用：**

- `hal/` 下的 `ecos/hal/*.h` —— SDK 内部接口，跨版本不承诺稳定
- `components/soc/*/include/*_soc.h` —— SoC 寄存器定义
- `board.h` 及任何引脚复用魔法数 —— 板级差异由 BSP 与 `board_resources.h` 屏蔽

## 错误模型

所有公共 API 的返回值遵循同一约定，核心类型为 `ecos_err_t`（`int32_t`，
见 [core.md](core.md)）：

- **0**（`ECOS_OK`）：成功；
- **正数**：成功且携带数据，例如读写字节数、`ecos_i2c_probe` 的 ACK 结果、
  `ecos_uart_try_read` 是否读到字节；
- **负数**：失败，取值为 `ECOS_ERR_*` 系列错误码
  （`ECOS_ERR_INVALID_ARGUMENT`、`ECOS_ERR_TIMEOUT` 等）。

辅助函数 `ecos_result_succeeded()` / `ecos_result_failed()` /
`ecos_err_is_known()` 用于判定结果；`ecos_err_name()` /
`ecos_err_description()` 把错误码转成可读字符串。

控制流宏简化错误处理：

- `ECOS_RETURN_ON_ERROR(expr)`：失败时把错误码直接 return 给调用者；
- `ECOS_GOTO_ON_ERROR(expr, errvar, label)`：失败时记录到 `errvar` 并跳转清理；
- `ECOS_PANIC_ON_ERROR(tag, expr, operation)`：失败时打印错误并停机，
  示例程序普遍使用它。

## 配置结构体惯例

驱动与设备统一采用「配置结构体 + 默认值宏」的初始化方式：

- `ecos_<x>_config_t`：实例配置（时钟分频、周期、引脚等）；
- `ECOS_<X>_CONFIG_DEFAULT`：与配置结构体对应的默认初始化器；
- 总线型模块另有 `ECOS_<X>_DEFAULT` 宏直接选择实例 0。

设备驱动（`ecos/device/*`）的 config 通过总线实例 ID 指定挂载点，例如
`ecos_aht20_config_t` 里的 `i2c` 字段。**应用必须先初始化总线驱动**
（如 `ecos_i2c_init`），再调用设备驱动的 `ecos_<dev>_init`；
设备句柄（`ecos_<dev>_t`）由应用持有，内部含 `initialized` 标志。

## include 命名空间

| 头文件 | 内容 |
| --- | --- |
| `ecos/error.h` | 错误码类型与判定/控制流宏 |
| `ecos/log.h` | 日志与 panic |
| `ecos/driver/<mod>.h` | 外设驱动：gpio、uart、i2c、pwm、timer、qspi、archinfo、rng、crc、rcu、rtc |
| `ecos/device/<dev>.h` | 设备驱动：aht20、at24c64、buzzer、esp01s_at、sgp30、pcf8563、st7735、st7789、tm1650 |
| `ecos/bsp/<res>.h` | BSP：console、led、button |
| `ecos/board_resources.h` | 由 CLI 依板级清单生成（输出到工程的 `.ecos/generated/include/`），描述板载演示资源 |

## 标准应用骨架

`example/` 下的每个示例都是同一个形状：初始化控制台 → 用
`ECOS_PANIC_ON_ERROR` 处理每一步错误 → 用 `ECOS_LOGI` 输出进度 →
用 `ecos_timer_delay_ms` 做阻塞延时 → 末尾空转。

```c
#include "ecos/bsp/console.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#define LOG_TAG "app"

int main(void)
{
    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    (void)ECOS_LOGI(LOG_TAG, "application started");

    for (;;) {
        /* 周期性工作…… */
        ECOS_PANIC_ON_ERROR(LOG_TAG,
                            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 1000u),
                            "delay");
    }
}
```

可直接参考的最小示例：`example/get_start/hello`（控制台输出）、
`example/get_start/blink`（LED + 定时器）。

## 2.x 遗留头文件警告

仓库中与新头文件同目录或邻近位置仍保留一批 2.x 遗留头文件（如 `hal_*.h`、
旧版 `st7735.h`、`components/core/include/log.h` 等），它们**不属于 3.0 公共
API**，仅供尚未迁移的旧代码编译使用，新应用不要包含。

特别警告：legacy `hal/i2c/hal_i2c.h` 与 `hal/timer/hal_timer.h` 和新契约头文件
存在 `hal_i2c_config_t` / `hal_timer_config_t` **同名类型冲突**，不要在同一编译
单元中混用两代头文件。3.x 应用应当只包含 `ecos/driver/i2c.h` 与
`ecos/driver/timer.h`。
