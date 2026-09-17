# RCU 驱动

时钟/复位控制单元：配置分频比与控制位，回读状态寄存器。

头文件：`drivers/rcu/include/ecos/driver/rcu.h`。
示例：`example/peripherals/rcu/status`。

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
```

## 函数

```c
ecos_err_t ecos_rcu_init(ecos_rcu_id_t id, const ecos_rcu_config_t *config);
ecos_err_t ecos_rcu_deinit(ecos_rcu_id_t id);
ecos_err_t ecos_rcu_get_status(ecos_rcu_id_t id, uint32_t *status);
```

- `ecos_rcu_init()` — 配置分频比与 CTRL 控制位。
- `ecos_rcu_get_status()` — 回读 STAT 状态寄存器。
- `ecos_rcu_deinit()` — 清除 CTRL 控制位。

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
- 目前仅 ysyx-2512-1 与 ysyx-2512-2 提供该 IP；CL1-2512（T1-Pico）没有。
- 不要与 2.x 遗留头文件 `hal/rcu/hal_rcu.h` 混用。
