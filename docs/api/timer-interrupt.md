# Timer 周期中断

`ecos/driver/timer_interrupt.h` 为已初始化的 Timer 注册周期回调。回调注册前必须先
调用 `ecos_timer_init()`，并在使能全局中断后调用 `ecos_timer_start()`。

```c
const ecos_timer_config_t config = { 1000000u };
ecos_timer_init(ECOS_TIMER_DEFAULT, &config);
ecos_timer_register_callback(ECOS_TIMER_DEFAULT, handler, NULL, 1u);
ecos_interrupt_global_enable();
ecos_timer_start(ECOS_TIMER_DEFAULT);
```

当前迁移实现覆盖 T1-Pico Timer0。注册回调后，重新初始化、反初始化或使用同一
Timer 做轮询延时会返回 `ECOS_ERR_BUSY`。回调中只更新状态，注销前应先停止 Timer。
