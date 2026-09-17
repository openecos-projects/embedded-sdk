# RNG 驱动

硬件随机数发生器：配置种子后读取 32 位随机字。

头文件：`drivers/rng/include/ecos/driver/rng.h`。
示例：`example/peripherals/rng/random`。

## 类型与常量

```c
typedef enum {
    ECOS_RNG_ID_0 = 0,   /* 唯一实例 */
    ECOS_RNG_ID_COUNT
} ecos_rng_id_t;

#define ECOS_RNG_DEFAULT ((ecos_rng_id_t)ECOS_RNG_ID_0)

typedef struct {
    uint32_t seed;   /* 随机种子 */
} ecos_rng_config_t;

#define ECOS_RNG_CONFIG_DEFAULT { 1u }
```

## 函数

```c
ecos_err_t ecos_rng_init(ecos_rng_id_t id, const ecos_rng_config_t *config);
ecos_err_t ecos_rng_deinit(ecos_rng_id_t id);
ecos_err_t ecos_rng_read(ecos_rng_id_t id, uint32_t *value);
```

- `ecos_rng_init()` — 写入种子并使能发生器。
- `ecos_rng_read()` — 读一个 32 位随机字。
- `ecos_rng_deinit()` — 关闭发生器。

## 最小使用骨架

```c
#include "ecos/bsp/console.h"
#include "ecos/driver/rng.h"
#include "ecos/log.h"

#define LOG_TAG "rng"

int main(void)
{
    const ecos_rng_config_t config = ECOS_RNG_CONFIG_DEFAULT;
    uint32_t value;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_rng_init(ECOS_RNG_DEFAULT, &config),
                        "initialize RNG");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_rng_read(ECOS_RNG_DEFAULT, &value),
                        "read random word");
    (void)ECOS_LOGI(LOG_TAG, "random: 0x%08X", value);
    for (;;) { }
}
```

## 注意事项

- 该硬件是**可播种的伪随机源**：相同种子产生相同序列，不要当作真随机数
  （TRNG）用于安全场景。
- 播种后建议给硬件若干周期再读取第一个值（示例中用 `ecos_timer_delay_ms`
  延时 1 ms）。
- 目前仅 ysyx-2512-1 与 ysyx-2512-2 提供该 IP；CL1-2512（T1-Pico）没有。
- 不要与 2.x 遗留头文件 `hal/rng/hal_rng.h` 混用。
