# PCF8563 RTC 实时时钟示例

该示例通过公共 I2C Driver 驱动 PCF8563（7 位地址 `0x51`）：初始化总线并
探测器件后，先向 RTC 写入一个基准时间（2026-09-17 12:30:00），随后每秒
读回一次日期时间并打印，共读取 10 次。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | 控制器 |
| --- | --- | --- | --- |
| StarrySky L4C1 | `starrysky-l4-c1` 或 `l4c1` | 支持 | ysyx-2512-1 I2C0 |
| StarrySky L4C2 | `starrysky-l4-c2` 或 `l4c2` | 不支持 | 板级未引出 I2C |
| StarrySky L4C3 | `starrysky-l4-c3` 或 `l4c3` | 不支持 | 板级未引出 I2C |
| StartySky T1-Pico | `startysky-t1-pico` 或 `t1-pico` | 支持 | CL1-2512 I2C0 |

当前示例要求板卡提供 `console`、`i2c-bus` 资源，并要求对应 Target 支持 I2C。
StarrySky L4C2/L4C3 未在板级引出 I2C 总线，示例清单已通过
`unsupported_boards` 显式排除这两块板卡，创建工程时会被拒绝。

## 创建和构建

创建并构建工程：

```bash
ecos project create i2c-pcf8563 --board starrysky-l4-c1
cd i2c-pcf8563
ecos build
```

使用 StartySky T1-Pico 时，将板卡参数改为 `--board t1-pico`。

## 连接和运行

ysyx-2512-1 的 I2C0 SCL 复用到 GPIO0[27]、SDA 复用到 GPIO0[28]。PCF8563
模块接至该总线（注意后备电池与电平匹配）后即可运行。串口输出形如：

```text
[i2c-pcf8563] initial time written, reading 10 times
[i2c-pcf8563] [0] 2026-09-17 12:30:00 (weekday 4)
[i2c-pcf8563] [1] 2026-09-17 12:30:01 (weekday 4)
...
[i2c-pcf8563] done
```

驱动不做时区/闰年换算，年份为相对 2000 年的偏移（0-99），寄存器读写
均为 BCD 格式。
