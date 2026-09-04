# Hello 示例

该示例初始化板卡 Console，并通过公共日志接口输出一条包含
`Hello, World!` 的启动日志。它用于验证 SDK 工程创建、Target 构建、板级串口和
日志输出链路是否正常。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 |
| --- | --- | --- |
| StarrySky L4 | `starrysky-l4` 或 `l4` | 支持 |
| StartySky T1-Pico | `startysky-t1-pico` 或 `t1-pico` | 支持 |

当前示例要求板卡提供 `console` 资源。以上两块板卡均已提供对应的 UART Console BSP。

## 创建和构建

以 StarrySky L4 为例：

```bash
ecos project create hello --board starrysky-l4
cd hello
ecos build
```

使用 StartySky T1-Pico 时，将创建命令中的板卡参数改为 `--board t1-pico`。

构建完成后，固件产物位于工程的 `build/` 目录。烧录并连接板卡串口后，可以看到
包含以下内容的日志：

```text
Hello, World!
```
