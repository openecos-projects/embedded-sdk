# UART 驱动

`driver-uart` 是 SDK 3.0 中与具体开发板无关的公共 UART 接口。

- 应用需要直接访问 UART 时，可以包含 `ecos/driver/uart.h`。
- Driver 负责校验端口和配置、记录初始化状态，并将 HAL 错误映射为公共错误码。
- Driver 只进行原始字节输入输出，不选择开发板引脚，也不转换换行符。
- 开发板默认终端的绑定和文本行为由 BSP Console API 负责。
- 端口与 SoC 实现的对应关系由 Target 决定：在 ysyx-2512-1/ysyx-2512-2 上，
  `ECOS_UART_PORT_0` 是 16550 风格控制台串口（固定 115200 8N1），
  `ECOS_UART_PORT_1` 是 hp_uart 块（TX=GPIO0[25]，RX=GPIO0[26]，8N1，波特率可配）。

以下示例初始化 UART0，并直接发送原始字节：

```c
#include "ecos/driver/uart.h"

static void write_uart0(void)
{
    static const char message[] = "raw uart0\n";
    const ecos_uart_config_t config = ECOS_UART_CONFIG_DEFAULT;

    if (ecos_uart_init(ECOS_UART_PORT_0, &config) == ECOS_OK)
        (void)ecos_uart_write(ECOS_UART_PORT_0, message, sizeof(message) - 1u);
}
```
