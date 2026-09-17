# CRC 校验驱动

`driver-crc` 是 SDK 3.0 面向应用的硬件 CRC 接口。应用包含
`ecos/driver/crc.h`，不直接包含内部 HAL 头文件或访问 SoC 寄存器。

## 接口

- `ecos_crc_init()`：配置初值、最终异或值和模式，并开始新一轮计算。
- `ecos_crc_feed()`：逐 32 位字喂入数据。
- `ecos_crc_read()`：读出当前计算结果。
- `ecos_crc_compute()`：便捷接口，依次喂入一个字数组后读出结果。
- `ecos_crc_deinit()`：关闭 CRC 单元。

`ECOS_CRC_DEFAULT` 选择唯一实例。`ECOS_CRC_CONFIG_DEFAULT` 是板测验证过的
组合（初值 0xFFFF、最终异或 0、模式 2）。

## ysyx-2512 实现

ysyx-2512-1 与 ysyx-2512-2 均提供该 IP（寄存器起始于 0x10301000），按 32 位字
喂入。`mode` 字段对应 CTRL bits[6:5]，各模式的多项式与位宽由 SoC 定义。

```c
#include "ecos/driver/crc.h"

const ecos_crc_config_t config = ECOS_CRC_CONFIG_DEFAULT;
const uint32_t data[] = { 0x00123456u };
uint32_t result;

if (ecos_crc_init(ECOS_CRC_DEFAULT, &config) == ECOS_OK &&
    ecos_crc_compute(ECOS_CRC_DEFAULT, data, 1u, &result) == ECOS_OK) {
    /* ... */
}
```
