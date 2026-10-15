# GPIO 外部中断

`ecos/driver/gpio_interrupt.h` 在 GPIO 驱动之上提供边沿/电平触发和回调注册。先用
`ecos_gpio_configure()` 把引脚设为输入，再设置触发类型、注册回调并使能中断。

```c
const ecos_gpio_pin_t input = ECOS_BOARD_GPIO_DEMO_INPUT;
ecos_gpio_interrupt_set_trigger(
    input.port, input.pin, ECOS_GPIO_INTERRUPT_ANYEDGE
);
ecos_gpio_interrupt_handler_add(input.port, input.pin, handler, NULL, 1u);
ecos_gpio_interrupt_enable(input.port, input.pin);
```

示例需包含 `ecos/board_resources.h`，并在 Example 清单中要求 `gpio-demo` 资源。
当前实现只把 T1-Pico GPIOA0..GPIOA7 映射到独立 PLIC source；其它 Board 会在配置阶段
报告缺少 `gpio-interrupt` 能力。
