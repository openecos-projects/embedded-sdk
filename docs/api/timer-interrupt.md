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

当前实现覆盖 T1-Pico Timer0 和实验性 CH32V103 TIM2（公共实例号均为 0）。
注册回调后，重新初始化、反初始化或使用同一 Timer 做轮询延时会返回
`ECOS_ERR_BUSY`。回调中只更新状态，注销前应先停止 Timer。

CH32V103 使用 PFIC IRQ44 和 TIM2 update flag：

- 必须先初始化且停止 TIM2，再注册回调；运行中注册返回 `ECOS_ERR_BUSY`。
- 支持 `1..536870912` 微秒周期。16-bit PSC/CNT 对长周期使用 2 的幂微秒量子，
  周期向上取整，误差小于一个量子。1 秒配置对应 PSC=127、ARR=62499。
- `start()` 重置 CNT 和预分频器，清 UIF，再开始完整周期。
- 回调前清 UIF（W0C，写 0 清除），不会误清其他 Timer 标志；虚假中断不调用回调。
- 注销屏蔽 UIE、禁用并释放 IRQ44、清除 pending，释放后可再次轮询或注册。
- 固定 HSI/APB1 8 MHz；更换时钟需重新设计时基，不能沿用本实现假设。

寄存器模型和交叉编译已覆盖这些路径，真实 PFIC 进入/返回和周期精度尚需实测。
