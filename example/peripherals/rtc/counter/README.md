# RTC 计数器与闹钟示例

该示例使用公共 RTC Driver 配置计数分频（默认 50000，按 50 MHz 输入估算约
1 kHz 计数），归零计数器后设置 1000 个计数之后的闹钟；延时约 500 ms 再次
读取计数器观察增量，随后轮询直到闹钟触发。

该 IP 是自由运行计数器加闹钟比较，不是日历钟；日历功能请使用外置 RTC
器件（如 PCF8563）。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | IP |
| --- | --- | --- | --- |
| StarrySky L4C1 | `starrysky-l4-c1` 或 `l4c1` | 支持 | ysyx-2512-1 RTC |
| StarrySky L4C2 | `starrysky-l4-c2` 或 `l4c2` | 支持 | ysyx-2512-1 RTC |
| StarrySky L4C3 | `starrysky-l4-c3` 或 `l4c3` | 支持 | ysyx-2512-2 RTC |
| StarrySky T1-Pico | `starrysky-t1-pico` 或 `t1-pico` | 不支持 | CL1-2512 无此 IP |

当前示例要求板卡提供 `console` 资源，并要求对应 Target 具备 `rtc` 能力；
CL1-2512（T1-Pico）未声明该能力，创建工程时会被拒绝。

## 创建和构建

```bash
ecos project create rtc-counter --board starrysky-l4-c1
cd rtc-counter
ecos build
```

使用 L4C2/L4C3 时，将板卡参数改为 `--board l4c2` 或 `--board l4c3`。

## 运行

无需接线，运行后串口输出形如：

```text
[rtc-counter] counter: 0 -> 500
[rtc-counter] alarm triggered
```

计数增量取决于 RTC 实际输入时钟；如果增量偏离预期，请按 SoC 数据手册调整
`main.c` 中的 `RTC_COUNTER_PRESCALER`。
