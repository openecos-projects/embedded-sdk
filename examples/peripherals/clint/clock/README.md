# CLINT 时钟读取示例

例程名：`clint-clock`。当前兼容板卡：StarrySky T1-Pico（`starrysky-t1-pico`）。
本例通过公共 CLINT Driver 读取一次 `mtime` 和时钟频率，输出后停留在空循环。
无需外接测试信号；需连接板卡供电、下载器及 UART 控制台。

## 创建、构建和运行

```bash
ecos project create clint-clock --board starrysky-t1-pico
cd clint-clock
ecos build
ecos flash
ecos monitor
```

T1-Pico 控制台默认 115200 波特率。必要时使用 `ecos monitor --port /dev/ttyUSB0`。
先打开监视终端，再复位板卡，以免错过一次性输出。
使用 CI BIN 时选择 `clint-clock-starrysky-t1-pico.bin`，通过板卡下载流程烧录，
不要将 `ecos flash` 当作任意 BIN 路径参数接口。

## 测试步骤与预期结果

1. 烧录后复位，确认出现包含以下内容的日志：

   ```text
   mtime: 0xHHHHHHHHLLLLLLLL (F Hz)
   ```

   `H`、`L` 为实际计数值，`F` 为驱动报告的频率，不是固定字符串。
2. 核对频率与该硬件/Target 的配置一致，无初始化失败或 panic。
3. 再次复位，应再次输出一条读数；随后不再打印是本例的正常行为。

本例只验证读取接口，不连续采样，不能据此证明计数单调性、时钟精度或定时中断正常。
记录测试日期、硬件版本、固件提交号/BIN SHA-256、实际读数与频率及通过/失败结论。
