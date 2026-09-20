# RCU 时钟复位驱动

`driver-rcu` 是 SDK 3.0 面向应用的时钟/复位控制接口。应用包含
`ecos/driver/rcu.h`，不直接包含内部 HAL 头文件或访问 SoC 寄存器。

## 接口

- `ecos_rcu_init()`：配置分频比（`clock_divider`，硬件按 N-1 写入 RDIV）和
  CTRL 控制位。
- `ecos_rcu_get_status()`：回读 STAT 状态寄存器。
- `ecos_rcu_deinit()`：清除 CTRL 控制位。
- `ecos_rcu_pll_apply()`：配置 PLL 并切换 CPU/AXI/AHB(/APB) 时钟域。
- `ecos_rcu_pll_get_freq()`：按寄存器回读计算指定时钟域频率。

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

## cl1-2512 PLL 实现

cl1-2512（T1-Pico）无 RDIV/CTRL 寄存器，`ecos_rcu_init()`/`ecos_rcu_deinit()`
返回 `ECOS_ERR_UNSUPPORTED`，但提供 CRU/PLL 时钟切换：

- `ecos_rcu_pll_apply()` 校验参数后按硬件序列切换时钟：Fvco = 20MHz × N ×
  (SELECT ? 2 : 1) 必须落在 500~1200MHz，CLK_DIV 的 APB 字节保持 0x08，
  PLL 稳定后 `keep_apb` 决定 APB 是否保持原时钟。
- `ecos_rcu_pll_get_freq()` 回读 PLL_CTRL0/2、CLK_DIV 与 CLK_CTRL 计算各
  时钟域频率；PLL 未使能时各域按 20MHz 参考时钟上报。

`ECOS_RCU_PLL_CONFIG_DEFAULT` 是已验证组合（N=40，SELECT×1，OD=/2，分频
2/2/4，APB 保持）：

| 时钟域 | 频率 |
| --- | --- |
| Fvco | 800 MHz |
| PLL | 400 MHz |
| CPU / AXI | 200 MHz |
| AHB / SDRAM | 100 MHz |
| APB | 20 MHz（保持，UART 不受影响） |

无 PLL 的 SoC（ysyx-2512-1/2）调用 PLL 接口返回 `ECOS_ERR_UNSUPPORTED`。

```c
#include "ecos/driver/rcu.h"

const ecos_rcu_pll_config_t pll_config = ECOS_RCU_PLL_CONFIG_DEFAULT;
uint32_t freq;

if (ecos_rcu_pll_apply(ECOS_RCU_DEFAULT, &pll_config) == ECOS_OK &&
    ecos_rcu_pll_get_freq(ECOS_RCU_DEFAULT, ECOS_RCU_CLOCK_CPU, &freq) == ECOS_OK) {
    /* ... */
}
```
