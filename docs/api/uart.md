# UART 驱动

阻塞式串口字节流收发。

头文件：`drivers/uart/include/ecos/driver/uart.h`。
示例：`example/peripherals/uart/esp01s`（经 ESP01S 设备驱动间接使用 UART）。

## 类型与常量

```c
typedef enum {
    ECOS_UART_PORT_0 = 0,   /* 控制台串口（sys UART） */
    ECOS_UART_PORT_1,       /* hp 块串口（StarrySky L4 上接 ESP01S） */
    ECOS_UART_PORT_COUNT
} ecos_uart_port_t;

typedef enum {
    ECOS_UART_PARITY_NONE = 0,  /* 无校验 */
    ECOS_UART_PARITY_ODD,       /* 奇校验 */
    ECOS_UART_PARITY_EVEN       /* 偶校验 */
} ecos_uart_parity_t;

typedef struct {
    uint32_t baud_rate;          /* 波特率 */
    uint8_t data_bits;           /* 数据位 */
    uint8_t stop_bits;           /* 停止位 */
    ecos_uart_parity_t parity;   /* 校验 */
} ecos_uart_config_t;

#define ECOS_UART_CONFIG_DEFAULT \
    { 115200u, 8u, 1u, ECOS_UART_PARITY_NONE }   /* 115200 8N1 */
```

## 函数

```c
ecos_err_t ecos_uart_init(ecos_uart_port_t port, const ecos_uart_config_t *config);
int ecos_uart_write(ecos_uart_port_t port, const void *data, size_t size);
int ecos_uart_read(ecos_uart_port_t port, void *data, size_t size);
int ecos_uart_try_read(ecos_uart_port_t port, uint8_t *data);
```

- `ecos_uart_init()` — 初始化指定串口实例，返回 `ECOS_OK` 或负错误码。
- `ecos_uart_write()` / `ecos_uart_read()` — 阻塞收发，返回实际传输的字节数，
  失败返回负错误码。
- `ecos_uart_try_read()` — 非阻塞读一个字节：读到返回 1，无数据返回 0，失败
  返回负错误码。

## 最小使用骨架

```c
#include "ecos/bsp/console.h"
#include "ecos/driver/uart.h"
#include "ecos/log.h"

#define LOG_TAG "uart"

int main(void)
{
    const ecos_uart_config_t config = ECOS_UART_CONFIG_DEFAULT;
    const char message[] = "ping\n";
    uint8_t byte;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_uart_init(ECOS_UART_PORT_1, &config),
                        "initialize UART1");

    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_uart_write(ECOS_UART_PORT_1,
                                        message, sizeof(message) - 1u),
                        "write UART1");

    for (;;) {
        int result = ecos_uart_try_read(ECOS_UART_PORT_1, &byte);

        ECOS_PANIC_ON_ERROR(LOG_TAG, result, "read UART1");
        if (result == 1)
            (void)ECOS_LOGI(LOG_TAG, "received 0x%02X", byte);
    }
}
```

## 注意事项

- 纯阻塞字节流：**没有中断、环形缓冲或 DMA API**；需要持续接收时请轮询
  `try_read` 或在专用任务中阻塞 `read`。
- `ECOS_UART_PORT_0` 是控制台串口，`bsp_console_*` 与 `printf` 都走它；应用再
  对它 `init` 会重配控制台参数。
- `ECOS_UART_PORT_1` 是 hp 块串口，StarrySky L4 板上连接到 ESP01S WiFi 模组，
  一般经 [devices.md](devices.md) 的 `esp01s_at` 驱动使用。
- 文本控制台语义（CRLF、CR→LF 归一化）只在 BSP console 层；UART 驱动收发的是
  原始字节。
