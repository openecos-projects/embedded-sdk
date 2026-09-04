# Blink 示例

该示例初始化板卡 Console、LED BSP 和 Timer Driver，使板载 LED 0 以 500 ms 的
间隔交替点亮和熄灭，并通过串口输出初始化及运行日志。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | 说明 |
| --- | --- | --- | --- |
| StarrySky L4 | `starrysky-l4` 或 `l4` | 支持 | LED 0 为 `GPIO1[5]`，低电平点亮 |
| StartySky T1-Pico | `startysky-t1-pico` 或 `t1-pico` | 支持 | LED 0 为 `GPIOD4`，低电平点亮 |

当前示例要求板卡同时提供 `console`、`led` 资源，并要求对应 Target 支持 Timer。
以上两块板卡均已提供相应的 LED BSP 和 Timer HAL。

## 创建和构建

```bash
ecos project create blink --board starrysky-l4
cd blink
ecos build
```

使用 StartySky T1-Pico 时，将创建命令中的板卡参数改为 `--board t1-pico`。

构建完成后，固件产物位于工程的 `build/` 目录。烧录并连接板卡串口后，LED 0
每 500 ms 切换一次状态，串口会输出 Blink 启动及运行信息。
