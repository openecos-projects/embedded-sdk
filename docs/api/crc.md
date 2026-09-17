# CRC 驱动

硬件 CRC 校验：配置初值/异或值/模式后按 32 位字喂入数据并读出结果。

头文件：`drivers/crc/include/ecos/driver/crc.h`。
示例：`example/peripherals/crc/compute`。

## 类型与常量

```c
typedef enum {
    ECOS_CRC_ID_0 = 0,   /* 唯一实例 */
    ECOS_CRC_ID_COUNT
} ecos_crc_id_t;

#define ECOS_CRC_DEFAULT ((ecos_crc_id_t)ECOS_CRC_ID_0)

typedef struct {
    uint32_t init_value;   /* 初值 */
    uint32_t xor_out;      /* 最终异或值 */
    uint8_t mode;          /* 模式 0-3，含义由 SoC 定义 */
} ecos_crc_config_t;

#define ECOS_CRC_CONFIG_DEFAULT { 0xFFFFu, 0u, 2u }   /* 板测验证过的组合 */
```

## 函数

```c
ecos_err_t ecos_crc_init(ecos_crc_id_t id, const ecos_crc_config_t *config);
ecos_err_t ecos_crc_deinit(ecos_crc_id_t id);
ecos_err_t ecos_crc_feed(ecos_crc_id_t id, uint32_t word);
ecos_err_t ecos_crc_read(ecos_crc_id_t id, uint32_t *result);
ecos_err_t ecos_crc_compute(ecos_crc_id_t id, const uint32_t *words,
                            size_t count, uint32_t *result);
```

- `ecos_crc_init()` — 装载初值/异或值/模式并开始新一轮计算。
- `ecos_crc_feed()` — 喂入一个 32 位字。
- `ecos_crc_read()` — 读出当前结果。
- `ecos_crc_compute()` — 便捷接口：依次喂入字数组后读出结果。

## 最小使用骨架

```c
#include "ecos/bsp/console.h"
#include "ecos/driver/crc.h"
#include "ecos/log.h"

#define LOG_TAG "crc"

int main(void)
{
    const ecos_crc_config_t config = ECOS_CRC_CONFIG_DEFAULT;
    const uint32_t data[] = { 0x00123456u };
    uint32_t result;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_crc_init(ECOS_CRC_DEFAULT, &config),
                        "initialize CRC");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_crc_compute(ECOS_CRC_DEFAULT, data, 1u, &result),
                        "compute CRC");
    (void)ECOS_LOGI(LOG_TAG, "CRC: 0x%08X", result);
    for (;;) { }
}
```

## 注意事项

- `mode` 对应 CTRL 寄存器 bits[6:5]，各模式的多项式与位宽由 SoC 定义；
  默认配置是板测验证过的组合，修改 `mode` 前请查阅数据手册。
- 每轮计算从 `ecos_crc_init()` 开始；连续计算需重新 init。
- 目前仅 ysyx-2512-1 与 ysyx-2512-2 提供该 IP；CL1-2512（T1-Pico）没有。
- 不要与 2.x 遗留头文件 `hal/crc/hal_crc.h` 混用。
