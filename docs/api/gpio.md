# GPIO 驱动

通用数字 IO 引脚控制：方向、电平与引脚功能（复用）配置。

头文件：`drivers/gpio/include/ecos/driver/gpio.h`。
示例：`example/peripherals/gpio/basic`。

## 类型与常量

```c
typedef enum {
    ECOS_GPIO_PORT_0 = 0,
    ECOS_GPIO_PORT_1,
    ECOS_GPIO_PORT_2,
    ECOS_GPIO_PORT_3,
    ECOS_GPIO_PORT_COUNT
} ecos_gpio_port_t;                 /* GPIO 端口组；有效引脚范围由 Target 决定 */

typedef enum {
    ECOS_GPIO_DIRECTION_INPUT = 0,  /* 输入 */
    ECOS_GPIO_DIRECTION_OUTPUT      /* 输出 */
} ecos_gpio_direction_t;

typedef enum {
    ECOS_GPIO_LEVEL_LOW = 0,        /* 低电平 */
    ECOS_GPIO_LEVEL_HIGH            /* 高电平 */
} ecos_gpio_level_t;

typedef enum {
    ECOS_GPIO_FUNCTION_GPIO = 0,    /* 普通 GPIO */
    ECOS_GPIO_FUNCTION_ALT_0,       /* 复用功能 0 */
    ECOS_GPIO_FUNCTION_ALT_1        /* 复用功能 1 */
} ecos_gpio_function_t;

typedef struct {
    ecos_gpio_port_t port;          /* 端口组 */
    uint8_t pin;                    /* 组内引脚号 */
} ecos_gpio_pin_t;

typedef struct {
    ecos_gpio_direction_t direction; /* 方向 */
    ecos_gpio_function_t function;   /* 引脚功能 */
} ecos_gpio_config_t;

#define ECOS_GPIO_CONFIG_DEFAULT \
    { ECOS_GPIO_DIRECTION_INPUT, ECOS_GPIO_FUNCTION_GPIO }  /* 输入 + GPIO 功能 */
```

## 函数

```c
ecos_err_t ecos_gpio_configure(ecos_gpio_port_t port, uint8_t pin,
                               const ecos_gpio_config_t *config);
ecos_err_t ecos_gpio_set_direction(ecos_gpio_port_t port, uint8_t pin,
                                   ecos_gpio_direction_t direction);
ecos_err_t ecos_gpio_set_level(ecos_gpio_port_t port, uint8_t pin,
                               ecos_gpio_level_t level);
int ecos_gpio_get_level(ecos_gpio_port_t port, uint8_t pin);
ecos_err_t ecos_gpio_set_function(ecos_gpio_port_t port, uint8_t pin,
                                  ecos_gpio_function_t function);
```

- `ecos_gpio_configure()` — 一次性配置方向和功能；有效引脚范围由所选 Target
  定义。
- `ecos_gpio_set_direction()` / `ecos_gpio_set_function()` — 单独改方向或复用
  功能。
- `ecos_gpio_set_level()` — 输出高低电平。
- `ecos_gpio_get_level()` — 返回 `ECOS_GPIO_LEVEL_LOW`(0) / `ECOS_GPIO_LEVEL_HIGH`
  (1)，失败返回负错误码。

## 最小使用骨架

```c
#include "ecos/bsp/console.h"
#include "ecos/driver/gpio.h"
#include "ecos/log.h"

#define LOG_TAG "gpio"

int main(void)
{
    const ecos_gpio_config_t out_cfg = {
        .direction = ECOS_GPIO_DIRECTION_OUTPUT,
        .function = ECOS_GPIO_FUNCTION_GPIO,
    };
    int level;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_gpio_configure(ECOS_GPIO_PORT_1, 5u, &out_cfg),
                        "configure output pin");

    for (;;) {
        level = ecos_gpio_get_level(ECOS_GPIO_PORT_1, 7u);
        ECOS_PANIC_ON_ERROR(LOG_TAG, level, "read input pin");
        ECOS_PANIC_ON_ERROR(LOG_TAG,
                            ecos_gpio_set_level(ECOS_GPIO_PORT_1, 5u,
                                                (ecos_gpio_level_t)level),
                            "write output pin");
    }
}
```

## 注意事项

- GPIO 没有 `get_instance_count` 与 init/deinit：引脚随用随配，无需全局初始化。
- 移植代码时优先使用 `ecos/board_resources.h` 的 `ecos_gpio_pin_t` 初始化器宏
  （见 [bsp.md](bsp.md)），避免硬编码引脚号。
- 位 bang 协议（如 TM1650）直接基于本模块实现，时序敏感的场合注意每次
  `set_level` 是一次完整的寄存器写。
