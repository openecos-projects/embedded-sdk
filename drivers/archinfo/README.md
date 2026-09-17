# Archinfo 芯片标识驱动

`driver-archinfo` 是 SDK 3.0 面向应用的芯片标识只读接口。应用包含
`ecos/driver/archinfo.h`，不直接包含内部 HAL 头文件或访问 SoC 寄存器。

## 接口

- `ecos_archinfo_get_system_id()`：读取 32 位系统标识字（SYS 寄存器）。
- `ecos_archinfo_get_chip_id()`：读取 64 位芯片 ID（IDH:IDL 拼接）。

标识寄存器只读，驱动没有 init/deinit 生命周期；调用前无需任何配置。
`ECOS_ARCHINFO_DEFAULT` 选择唯一的 Archinfo 实例。

## ysyx-2512 实现

ysyx-2512-1 与 ysyx-2512-2 均提供该 IP，寄存器位于 0x10006000 起始。
SYS/IDL/IDH 三个字段的具体编码由 SoC 定义。

```c
#include "ecos/driver/archinfo.h"

uint64_t chip_id;
if (ecos_archinfo_get_chip_id(ECOS_ARCHINFO_DEFAULT, &chip_id) == ECOS_OK) {
    /* ... */
}
```
