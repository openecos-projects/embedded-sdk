# Archinfo 驱动

读取 SoC 的系统标识字与芯片 ID（只读寄存器组）。

头文件：`drivers/archinfo/include/ecos/driver/archinfo.h`。
示例：`example/peripherals/archinfo/chip-id`。

## 类型与常量

```c
typedef enum {
    ECOS_ARCHINFO_ID_0 = 0,   /* 唯一实例 */
    ECOS_ARCHINFO_ID_COUNT
} ecos_archinfo_id_t;

#define ECOS_ARCHINFO_DEFAULT ((ecos_archinfo_id_t)ECOS_ARCHINFO_ID_0)
```

## 函数

```c
ecos_err_t ecos_archinfo_get_system_id(ecos_archinfo_id_t id, uint32_t *value);
ecos_err_t ecos_archinfo_get_chip_id(ecos_archinfo_id_t id, uint64_t *value);
```

- `ecos_archinfo_get_system_id()` — 读 32 位系统标识字（SYS 寄存器）。
- `ecos_archinfo_get_chip_id()` — 读 64 位芯片 ID（IDH:IDL 拼接）。

## 最小使用骨架

```c
#include "ecos/bsp/console.h"
#include "ecos/driver/archinfo.h"
#include "ecos/log.h"

#define LOG_TAG "archinfo"

int main(void)
{
    uint32_t system_id;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_archinfo_get_system_id(ECOS_ARCHINFO_DEFAULT,
                                                    &system_id),
                        "read system id");
    (void)ECOS_LOGI(LOG_TAG, "system id: 0x%08X", system_id);
    for (;;) { }
}
```

## 注意事项

- 标识寄存器只读，**没有 init/deinit 生命周期**，调用前无需初始化。
- SYS/IDL/IDH 字段的具体编码由 SoC 定义，SDK 不做解释。
- 目前仅 ysyx-2512-1 与 ysyx-2512-2 提供该 IP；CL1-2512（T1-Pico）没有。
- 不要与 2.x 遗留头文件 `hal/archinfo/hal_archinfo.h` 混用。
