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
T1-Pico 将 GPIOA0..GPIOA7 映射到独立 PLIC source。

CH32V103 的实验性实现使用 AFIO + EXTI0..15：

- 每个编号的 EXTI line 只能选择一个 GPIO port（例如 PA0 和 PB0 不能同时注册）；
  已注册 line 被其他 port 占用时返回 `ECOS_ERR_BUSY`，未注册时可重新路由。
- 仅支持上升沿、下降沿、双边沿；低/高电平触发返回 `ECOS_ERR_UNSUPPORTED`。
- 先配置 GPIO 输入，再设置触发、注册、使能。修改触发配置会先禁用该 line，需重新使能。
- EXTI5..9、EXTI10..15 分别共享 PFIC 源，组内已注册的 line 必须使用相同优先级，
  不同优先级返回 `ECOS_ERR_BUSY`；一个 line 禁用/移除不会误关同组其他 line。
- 分发器只处理 pending 且已使能的 line，先按 W1C 清除本次快照，再调用回调，
  避免回调期间的新边沿被随后的统一清除操作丢失。
- CH32 Board 的 `gpio-interrupt` 示例使用 PA0 外接输入，不是已确认的板载按键；
  需外接稳定上拉/下拉，不要悬空。GPIO 封装可用引脚仍以实物为准。

本次覆盖寄存器模型和构建测试，实际边沿触发仍需上板验证。
其他未声明 `gpio-interrupt` 能力的 Target 会在工程创建阶段拒绝此示例。
