# SGP30 空气质量传感器示例

该示例通过公共 I2C Driver 驱动 SGP30（7 位地址 `0x58`）：初始化总线后启动
IAQ 测量，读取 48 位序列号，随后每秒测量一次 CO2 当量（ppm）与 TVOC
（ppb），共采样 20 次。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | 控制器 |
| --- | --- | --- | --- |
| StarrySky L4C1 | `starrysky-l4-c1` 或 `l4c1` | 支持 | ysyx-2512-1 I2C0 |
| StarrySky L4C2 | `starrysky-l4-c2` 或 `l4c2` | 不支持 | 板级未引出 I2C |
| StarrySky L4C3 | `starrysky-l4-c3` 或 `l4c3` | 不支持 | 板级未引出 I2C |
| StarrySky T1-Pico | `starrysky-t1-pico` 或 `t1-pico` | 支持 | CL1-2512 I2C0 |

当前示例要求板卡提供 `console`、`i2c-bus` 资源，并要求对应 Target 支持 I2C。
StarrySky L4C2/L4C3 未在板级引出 I2C 总线，示例清单已通过
`unsupported_boards` 显式排除这两块板卡，创建工程时会被拒绝。

## 创建和构建

创建并构建工程：

```bash
ecos project create i2c-sgp30 --board starrysky-l4-c1
cd i2c-sgp30
ecos build
```

使用 StarrySky T1-Pico 时，将板卡参数改为 `--board t1-pico`。

## 连接和运行

ysyx-2512-1 的 I2C0 SCL 复用到 GPIO0[27]、SDA 复用到 GPIO0[28]。SGP30 模块
接至该总线并确认电平兼容后即可运行。串口输出形如：

```text
[i2c-sgp30] SGP30 serial: 0x00000064 0x00ABCDEF, sampling 20 times
[i2c-sgp30] note: IAQ readings are defaults for the first 15 s after init
[i2c-sgp30] [0] CO2eq: 400 ppm, TVOC: 0 ppb
...
[i2c-sgp30] [19] CO2eq: 512 ppm, TVOC: 7 ppb
[i2c-sgp30] done
```

注意：SGP30 启动 IAQ 测量后的前 15 秒返回默认值（CO2eq 400 ppm、
TVOC 0 ppb），属芯片正常行为。
