# RCU 时钟复位驱动

`driver-rcu` 是 SDK 3.0 面向应用的时钟/复位控制接口。应用包含
`ecos/driver/rcu.h`，不直接包含内部 HAL 头文件或访问 SoC 寄存器。

## 接口

- `ecos_rcu_init()`：配置分频比（`clock_divider`，硬件按 N-1 写入 RDIV）和
  CTRL 控制位。
- `ecos_rcu_get_status()`：回读 STAT 状态寄存器。
- `ecos_rcu_deinit()`：清除 CTRL 控制位。

`ECOS_RCU_DEFAULT` 选择唯一实例。注意 RCU 影响时钟与复位域，错误的配置可能
导致系统异常，一般只在硬件验证代码中使用。

## ysyx-2512 实现

ysyx-2512-1 与 ysyx-2512-2 均提供该 IP（寄存器起始于 0x10002000）。
CTRL 各位控制的时钟/复位域由 SoC 定义，SDK 不做解释。

```c
#include "ecos/driver/rcu.h"

const ecos_rcu_config_t config = ECOS_RCU_CONFIG_DEFAULT;
uint32_t status;

if (ecos_rcu_init(ECOS_RCU_DEFAULT, &config) == ECOS_OK &&
    ecos_rcu_get_status(ECOS_RCU_DEFAULT, &status) == ECOS_OK) {
    /* ... */
}
```
