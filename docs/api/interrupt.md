# 机器中断驱动

`ecos/driver/interrupt.h` 提供 PLIC source 的分配、优先级、使能和机器全局中断控制。
回调应只做短小、可重入的工作；串口输出和复杂设备访问应放到主循环。

```c
static volatile unsigned count;
static void handler(void *arg) { (void)arg; ++count; }

ecos_interrupt_init();
ecos_interrupt_alloc(3u, 1u, handler, NULL);
ecos_interrupt_enable(3u);
ecos_interrupt_global_enable();
```

PLIC source 号和可用范围由 Target 决定。当前 3.0 迁移实现覆盖 T1-Pico；应用不应
包含 SoC 头文件或直接读写 PLIC 寄存器。
