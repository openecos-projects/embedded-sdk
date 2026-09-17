# RNG 随机数驱动

`driver-rng` 是 SDK 3.0 面向应用的随机数接口。应用包含
`ecos/driver/rng.h`，不直接包含内部 HAL 头文件或访问 SoC 寄存器。

## 接口

- `ecos_rng_init()`：写入种子并使能随机数发生器。
- `ecos_rng_read()`：读取一个 32 位随机字。
- `ecos_rng_deinit()`：关闭发生器。

`ECOS_RNG_DEFAULT` 选择唯一实例；`ECOS_RNG_CONFIG_DEFAULT` 提供默认种子。

## ysyx-2512 实现

ysyx-2512-1 与 ysyx-2512-2 均提供该 IP（寄存器起始于 0x10300000）。该硬件是
可播种的伪随机源：相同种子产生相同序列，不要当作真随机数（TRNG）用于安全场景。

```c
#include "ecos/driver/rng.h"

const ecos_rng_config_t config = ECOS_RNG_CONFIG_DEFAULT;
uint32_t value;

if (ecos_rng_init(ECOS_RNG_DEFAULT, &config) == ECOS_OK &&
    ecos_rng_read(ECOS_RNG_DEFAULT, &value) == ECOS_OK) {
    /* ... */
}
```
