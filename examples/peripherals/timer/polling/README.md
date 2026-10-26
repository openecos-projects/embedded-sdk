# Timer 轮询基础测试

该示例使用 CH32V103 的 TIM2 验证基础定时器轮询链路：

- 初始化 TIM2；
- 使用 HSI 8 MHz、1 MHz timer tick；
- 启动/停止定时器；
- 轮询 update flag；
- 连续观察 5 次 10 ms 周期；
- 每次到期翻转 Board `gpio-demo` 的输出引脚，并通过串口输出结果。

## CH32V103

```bash
ecos project create timer-polling \
  --board ch32v103c8t6 \
  --name timer-polling-ch32
cd timer-polling-ch32
ecos build
ecos flash
ecos monitor --port <串口设备> --expect "timer polling completed"
```

预期日志包含：

```text
[I][timer-polling] TIM2 polling ready: 10 ms period, HSI 8 MHz
[I][timer-polling] period elapsed: 1, output=HIGH
...
[I][timer-polling] period elapsed: 5, output=HIGH
[I][timer-polling] timer polling completed
```

该示例是轮询测试，不验证 PFIC、Timer 中断、PLL 或 HSI 频率精度。CH32V103 上
PA1 会每 10 ms 翻转一次，可用示波器或逻辑分析仪观察约 20 ms 的完整方波周期；PA0
保留为 GPIO 基础测试输入。
