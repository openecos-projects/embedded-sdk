# GPIO 驱动

`driver-gpio` 是 SDK 3.0 中与具体开发板无关的公共 GPIO 接口。

- 应用包含 `ecos/driver/gpio.h`，不直接访问 HAL 或 SoC 寄存器。
- Driver 负责校验端口、引脚和配置，并将 HAL 错误映射为公共错误码。
- StarrySky L4 提供 GPIO0、GPIO1 和 GPIO2 三个端口，每个端口包含 32 个引脚。
- PinMux 的具体信号绑定仍由 BSP 或设备配置决定；Driver 只表达 GPIO、复用功能 0 和复用功能 1。

以下示例将 GPIO0[0] 配置为普通 GPIO 输出并拉高：

```c
#include "ecos/driver/gpio.h"

static int set_gpio0_high(void)
{
    const ecos_gpio_config_t config = {
        ECOS_GPIO_DIRECTION_OUTPUT,
        ECOS_GPIO_FUNCTION_GPIO,
    };
    int result = ecos_gpio_configure(ECOS_GPIO_PORT_0, 0u, &config);

    if (result != ECOS_OK)
        return result;
    return ecos_gpio_set_level(
        ECOS_GPIO_PORT_0, 0u, ECOS_GPIO_LEVEL_HIGH
    );
}
```
