# 机器中断驱动

`ecos/driver/interrupt.h` 提供中断源分配、优先级、使能和机器全局中断控制。
当前实现覆盖 T1-Pico 的 PLIC 和 CH32V103 的 PFIC；两者中断源编号不能混用。
回调只做短小的计数/状态更新；日志、串口、阻塞延时放在主循环中执行。

```c
static volatile unsigned count;
static void handler(void *arg) { (void)arg; ++count; }

/* source 由 Target 决定；优先使用 GPIO/Timer 专用回调接口，避免硬编码。 */
ecos_interrupt_init();
ecos_interrupt_alloc(source, 1u, handler, NULL);
ecos_interrupt_set_threshold(0u);
ecos_interrupt_enable(source);
ecos_interrupt_global_enable();
```

注册不会自动使能中断源；使能中断源也不会自动打开全局中断。释放会禁用该中断源。
初始化是幂等的，不应清除已注册的处理函数。

## CH32V103 PFIC（实验性）

- `source` 使用 WCH IRQ 编号：SysTick=12、Software=14、外设 16..34 与 39..59；
  NMI=2、异常=3、保留编号和越界编号不能通过公共接口分配。
- TIM2 使用 IRQ44；EXTI0..4 使用 IRQ22..26；EXTI5..9 共用 IRQ39；
  EXTI10..15 共用 IRQ56。GPIO 和 Timer Driver 会管理对应中断源，不要重复占用。
- 公共优先级 `1..16`，值越大越紧急；HAL 转换为 PFIC 的反向编码（最高 4 位）。
  `set_threshold(n)` 屏蔽公共优先级 `<= n` 的源，0 放行全部；阈值通过源使能实现，
  不把 PFIC 的原始门限编码暴露为公共语义。没有中断抢占/嵌套。
- 启动代码在 `.data/.bss` 初始化后，若链接了 `interrupt` 能力，会安装 60 个
  **4 字节跳转槽位**的向量表，`mtvec MODE=1`；仍保持全局中断关闭。
- 使用普通 xPack RV32 ABI，入口在应用栈上保存 x1、x3..x31、`mepc/mstatus` 和故障
  信息（144 字节，保持 16 字节对齐），以 `mret` 返回。PFIC 硬件压栈、VTF 快速
  路径和中断嵌套关闭，不依赖 `WCH-Interrupt-fast` 编译属性，不保存 FPU 上下文。
- 回调必须清除所属外设的事件标志；PFIC 不是 PLIC，没有 claim/complete 寄存器。
  未注册或不应使能的中断源会被屏蔽，防止中断风暴。
- 同步异常/NMI 等不走普通回调。默认记录 `mcause/mepc/mtval` 到 Target 私有的
  `ecos_ch32v103_last_fault` 后停机，可在调试器检查。内部弱符号
  `ecos_ch32v103_exception_handler()` 可在需要时覆盖，但必须不返回；不是稳定公共 API。
- 必须在中断关闭时完成初始化，回调中不得再次打开全局中断、阻塞或切换执行上下文。
  不提供 RTOS 上下文切换、UART/SPI/I2C/DMA 异步驱动承诺。

PFIC/EXTI/TIM2 的寄存器模型测试和 ELF 向量/现场保存检查不是实机验证。
硬件仍需验证中断进入/返回、寄存器恢复、60 秒周期计数及 GPIO 边沿响应。
