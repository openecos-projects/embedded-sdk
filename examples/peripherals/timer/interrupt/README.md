# Timer 周期中断示例

例程名：`timer-interrupt`。兼容 StarrySky T1-Pico（`starrysky-t1-pico`）和实验性
CH32V103C8T6（`ch32v103c8t6`）。
使用公共 Timer 和中断接口启动默认定时器，在回调中增加计数，在主循环输出计数变化。
无需外部测试信号；连接板卡供电、下载器及 UART 控制台。

## 创建、构建和运行

```bash
ecos project create timer-interrupt --board starrysky-t1-pico
cd timer-interrupt
ecos build
ecos flash
ecos monitor
```

控制台默认 115200 波特率；必要时使用 `ecos monitor --port /dev/ttyUSB0`。
T1-Pico 的 CI BIN 为 `timer-interrupt-starrysky-t1-pico.bin`。
CH32 创建时改用 `--board ch32v103c8t6`，按其 wlink 流程烧录；公共 Timer0 对应
TIM2/PFIC IRQ44。打开终端后手动复位，以便查看初始化日志。

CH32 的 1 秒周期使用 PSC=127、ARR=62499，固定 HSI/APB1 8 MHz；该配置已完成
构建/寄存器模型验证，真实中断进入/返回、周期精度和稳定性尚待实测。

## 测试步骤与预期结果

1. 确认出现 `Timer0 interrupt ready`。
2. 随后持续出现计数，例如：

   ```text
   ticks: 1
   ticks: 2
   ticks: 3
   ```

3. 持续观察至少 60 秒，计数应继续增加，不应停滞、panic 或复位。
4. 断电重启后重复检查，计数从本次启动的初始状态重新开始。

源码设置 `period_us = 1000000`，期望周期为 1 秒；实际硬件周期仍需测量确认。
主循环可能合并多次中断的变化，日志计数不要求逐个连续。
若要验收周期精度或抖动，应另用时间戳或测量仪器。

只有 ready 而无后续 ticks 不算通过，应检查中断路径、时钟和阈值配置。
记录硬件版本、固件提交号/BIN SHA-256、观察时长、起止计数、日志及测试结论。

若自行改测试程序，先停止 Timer，再注销回调；注销后可反初始化、重新配置和注册。
回调占用期间，同一个 Timer 的重新初始化、反初始化和轮询延时应返回 `ECOS_ERR_BUSY`。
