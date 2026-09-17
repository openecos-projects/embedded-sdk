# BSP：控制台、LED、按键与板级资源

BSP（板级支持包）把具体板卡上的控制台串口、LED、按键和演示资源封装成与板卡
无关的接口，是应用最先初始化的模块。

头文件：`board/include/ecos/bsp/console.h`、`led.h`、`button.h`；
生成的 `ecos/board_resources.h`（由 CLI 依板级清单生成到工程的
`.ecos/generated/include/`）。

示例：`example/get_start/hello`、`example/get_start/blink`。

## 控制台 console

控制台是 CRLF 文本输出通道 + 阻塞文本输入通道，输入中的 CR 会归一化为 LF。

```c
ecos_err_t bsp_console_init(void);
int bsp_console_write(const char *text, size_t size);
int bsp_console_read(void *data, size_t size);
int bsp_console_try_read(uint8_t *data);
```

- `bsp_console_init()` — 初始化板卡选定的控制台，返回 0 或负错误码。几乎每个
  应用的第一行都是它。
- `bsp_console_write()` — 输出一段文本（按 CRLF 约定），返回消费的字节数，
  失败返回负错误码。
- `bsp_console_read()` — 阻塞读取直到读满 `size` 字节；回车（CR）被归一化为
  换行（LF）。返回读取的字节数或负错误码。
- `bsp_console_try_read()` — 非阻塞读一个字节：读到返回 1，暂无数据返回 0，
  失败返回负错误码。

## LED

```c
typedef enum {
    BSP_LED_0 = 0,
    BSP_LED_1,
    BSP_LED_COUNT
} bsp_led_t;

typedef enum {
    BSP_LED_OFF = 0,
    BSP_LED_ON
} bsp_led_state_t;

ecos_err_t bsp_led_init(void);
ecos_err_t bsp_led_set_state(bsp_led_t led, bsp_led_state_t state);
```

- `BSP_LED_0` / `BSP_LED_1` — 板载 LED 编号；`BSP_LED_COUNT` 为数量上界。
- `bsp_led_init()` — 把所有板载 LED 配置为 GPIO 输出并置为熄灭。
- `bsp_led_set_state()` — 点亮（`BSP_LED_ON`）或熄灭（`BSP_LED_OFF`）指定 LED。

## 按键 button

```c
typedef enum {
    BSP_BUTTON_0 = 0,
    BSP_BUTTON_1,
    BSP_BUTTON_COUNT
} bsp_button_t;

typedef enum {
    BSP_BUTTON_RELEASED = 0,
    BSP_BUTTON_PRESSED
} bsp_button_state_t;

ecos_err_t bsp_button_init(void);
int bsp_button_get_state(bsp_button_t button);
```

- `bsp_button_init()` — 把所有板载按键配置为 GPIO 输入。
- `bsp_button_get_state()` — 返回 `BSP_BUTTON_RELEASED`（0）或
  `BSP_BUTTON_PRESSED`（1），失败返回负错误码。无消抖、无中断，需要消抖时
  由应用轮询实现。

## 板级资源宏 board_resources.h

`ecos/board_resources.h` 由 `ecos` CLI 在配置工程时根据所选 Board 的资源清单
生成，不存在于 SDK 源码树中。每个资源组都以 `ECOS_BOARD_HAS_*` 作为存在性
开关，示例代码用 `#if !ECOS_BOARD_HAS_*` + `#error` 做编译期断言。

GPIO 演示资源（一组输入引脚 + 一组输出引脚，可回环对连）：

```c
#define ECOS_BOARD_HAS_GPIO_DEMO 1
#define ECOS_BOARD_GPIO_DEMO_INPUT  { ECOS_GPIO_PORT_1, 7u }  /* ecos_gpio_pin_t 初始化器 */
#define ECOS_BOARD_GPIO_DEMO_INPUT_LABEL "GPIO1[7]"           /* 可打印的引脚名 */
#define ECOS_BOARD_GPIO_DEMO_INPUT_IDLE_LEVEL ECOS_GPIO_LEVEL_HIGH   /* 输入空闲电平 */
#define ECOS_BOARD_GPIO_DEMO_OUTPUT { ECOS_GPIO_PORT_1, 5u }
#define ECOS_BOARD_GPIO_DEMO_OUTPUT_LABEL "GPIO1[5]"
#define ECOS_BOARD_GPIO_DEMO_OUTPUT_INITIAL_LEVEL ECOS_GPIO_LEVEL_HIGH /* 输出初始电平 */
```

QSPI 总线资源：

```c
#define ECOS_BOARD_HAS_QSPI_BUS 1
#define ECOS_BOARD_QSPI_BUS_CONTROLLER ((ecos_qspi_id_t)0u)  /* 控制器实例 */
#define ECOS_BOARD_QSPI_BUS_CLOCK_DIVIDER 3u                 /* 时钟分频 */
```

板载显示屏资源（配合 ST7735/ST7789 设备驱动的 config 结构体逐字段使用）：

```c
#define ECOS_BOARD_HAS_DISPLAY 1
#define ECOS_BOARD_DISPLAY_CHIP_SELECT ECOS_QSPI_CS_0   /* 片选 */
#define ECOS_BOARD_DISPLAY_DC_PORT ECOS_GPIO_PORT_0     /* D/C 引脚 */
#define ECOS_BOARD_DISPLAY_DC_PIN 29u
#define ECOS_BOARD_DISPLAY_RESET_PORT ECOS_GPIO_PORT_0  /* 复位引脚 */
#define ECOS_BOARD_DISPLAY_RESET_PIN 30u
#define ECOS_BOARD_DISPLAY_BACKLIGHT_PORT ECOS_GPIO_PORT_0 /* 背光引脚 */
#define ECOS_BOARD_DISPLAY_BACKLIGHT_PIN 31u
#define ECOS_BOARD_DISPLAY_WIDTH 128u          /* 面板宽度（像素） */
#define ECOS_BOARD_DISPLAY_HEIGHT 128u         /* 面板高度（像素） */
#define ECOS_BOARD_DISPLAY_ROTATION 0u         /* 旋转 */
#define ECOS_BOARD_DISPLAY_HORIZONTAL_OFFSET 2u /* 显存水平偏移 */
#define ECOS_BOARD_DISPLAY_VERTICAL_OFFSET 3u   /* 显存垂直偏移 */
```

## 最小使用骨架

```c
#include "ecos/bsp/console.h"
#include "ecos/bsp/led.h"
#include "ecos/driver/timer.h"
#include "ecos/log.h"

#define LOG_TAG "blink"

int main(void)
{
    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_led_init(), "initialize LED");

    for (;;) {
        ECOS_PANIC_ON_ERROR(LOG_TAG,
                            bsp_led_set_state(BSP_LED_0, BSP_LED_ON), "LED on");
        ECOS_PANIC_ON_ERROR(LOG_TAG,
                            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 500u),
                            "delay");
        ECOS_PANIC_ON_ERROR(LOG_TAG,
                            bsp_led_set_state(BSP_LED_0, BSP_LED_OFF), "LED off");
        ECOS_PANIC_ON_ERROR(LOG_TAG,
                            ecos_timer_delay_ms(ECOS_TIMER_DEFAULT, 500u),
                            "delay");
    }
}
```

## 注意事项

- `bsp_console_init()` 应最先调用：日志默认写到控制台，初始化失败时
  `ECOS_PANIC_ON_ERROR` 也能输出错误信息。
- 控制台写的是文本：`\n` 会被加上 `\r` 组成 CRLF；二进制通道请用 UART 驱动。
- `board_resources.h` 的具体取值随 Board 变化，跨板移植代码时只依赖宏名，
  不要硬编码引脚。
- LED/按键的具体数量由板卡决定，`BSP_LED_COUNT` / `BSP_BUTTON_COUNT` 之外
  的编号行为未定义。
