# RCU 驱动

时钟/复位控制单元：配置分频比与控制位，回读状态寄存器；在带 PLL 的 SoC 上
切换并查询各时钟域频率。

头文件：`drivers/rcu/include/ecos/driver/rcu.h`。
示例：`examples/peripherals/rcu/status`、`examples/peripherals/rcu/pll`。

## 类型与常量

```c
typedef enum {
    ECOS_RCU_ID_0 = 0,   /* 唯一实例 */
    ECOS_RCU_ID_COUNT
} ecos_rcu_id_t;

#define ECOS_RCU_DEFAULT ((ecos_rcu_id_t)ECOS_RCU_ID_0)

typedef struct {
    uint32_t clock_divider;   /* 分频比，硬件按 N-1 写入 RDIV */
    uint32_t control;         /* CTRL 原始位，位定义由 SoC 决定 */
} ecos_rcu_config_t;

#define ECOS_RCU_CONFIG_DEFAULT { 1u, 0u }

typedef enum {
    ECOS_RCU_PLL_SELECT_X1 = 0,   /* PLL 参考倍频 ×1 */
    ECOS_RCU_PLL_SELECT_X2 = 1
} ecos_rcu_pll_select_t;

typedef enum {
    ECOS_RCU_PLL_OD_1 = 0,   /* PLL 输出分频 /1 */
    ECOS_RCU_PLL_OD_2 = 1,
    ECOS_RCU_PLL_OD_4 = 2,
    ECOS_RCU_PLL_OD_8 = 3
} ecos_rcu_pll_od_t;

typedef enum {
    ECOS_RCU_CLOCK_PLL = 0,
    ECOS_RCU_CLOCK_CPU,
    ECOS_RCU_CLOCK_AXI,
    ECOS_RCU_CLOCK_AHB,
    ECOS_RCU_CLOCK_APB
} ecos_rcu_clock_domain_t;

typedef struct {
    uint8_t n;                     /* PLL 反馈分频比，>=1 */
    ecos_rcu_pll_select_t select;
    ecos_rcu_pll_od_t od;
    uint8_t cpu_div;               /* 各域分频字节，写入 CLK_DIV */
    uint8_t axi_div;
    uint8_t ahb_div;
    bool keep_apb;                 /* true = APB 保持原时钟 */
    uint32_t settle_ticks;         /* PLL 稳定等待，0 = 默认值 */
} ecos_rcu_pll_config_t;

/* cl1-2512 已验证组合 */
#define ECOS_RCU_PLL_CONFIG_DEFAULT \
    { 40u, ECOS_RCU_PLL_SELECT_X1, ECOS_RCU_PLL_OD_2, 2u, 2u, 4u, true, 0u }
```

## 函数

```c
ecos_err_t ecos_rcu_init(ecos_rcu_id_t id, const ecos_rcu_config_t *config);
ecos_err_t ecos_rcu_deinit(ecos_rcu_id_t id);
ecos_err_t ecos_rcu_get_status(ecos_rcu_id_t id, uint32_t *status);
ecos_err_t ecos_rcu_pll_apply(ecos_rcu_id_t id,
                              const ecos_rcu_pll_config_t *config);
ecos_err_t ecos_rcu_pll_get_freq(ecos_rcu_id_t id,
                                 ecos_rcu_clock_domain_t domain,
                                 uint32_t *freq_hz);
```

- `ecos_rcu_init()` — 配置分频比与 CTRL 控制位。
- `ecos_rcu_get_status()` — 回读 STAT 状态寄存器。
- `ecos_rcu_deinit()` — 清除 CTRL 控制位。
- `ecos_rcu_pll_apply()` — 校验 PLL 参数（Fvco 必须落在 500~1200MHz）后按
  硬件序列切换 CPU/AXI/AHB(/APB) 时钟域。
- `ecos_rcu_pll_get_freq()` — 按寄存器回读计算指定时钟域频率。

`ECOS_RCU_PLL_CONFIG_DEFAULT` 在 cl1-2512 上的各域频率：

| 时钟域 | 频率 |
| --- | --- |
| Fvco | 800 MHz |
| PLL | 400 MHz |
| CPU / AXI | 200 MHz |
| AHB / SDRAM | 100 MHz |
| APB | 20 MHz（保持，UART 不受影响） |

## 最小使用骨架

```c
#include "ecos/bsp/console.h"
#include "ecos/driver/rcu.h"
#include "ecos/log.h"

#define LOG_TAG "rcu"

int main(void)
{
    const ecos_rcu_config_t config = ECOS_RCU_CONFIG_DEFAULT;
    uint32_t status;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_rcu_init(ECOS_RCU_DEFAULT, &config),
                        "initialize RCU");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_rcu_get_status(ECOS_RCU_DEFAULT, &status),
                        "read RCU status");
    (void)ECOS_LOGI(LOG_TAG, "RCU status: 0x%08X", status);
    for (;;) { }
}
```

## 注意事项

- **RCU 影响时钟与复位域**，错误配置可能导致系统异常；一般只在硬件验证
  代码中使用，普通应用不需要调用。
- CTRL 各位控制的时钟/复位域由 SoC 定义，SDK 不做解释。
- ysyx-2512-1 与 ysyx-2512-2 提供 RDIV/CTRL 接口，PLL 接口返回
  `ECOS_ERR_UNSUPPORTED`；cl1-2512（T1-Pico）提供 PLL 接口，
  `ecos_rcu_init()`/`ecos_rcu_deinit()` 返回 `ECOS_ERR_UNSUPPORTED`。
- 不要与 2.x 遗留头文件 `hal/rcu/hal_rcu.h` 混用。
