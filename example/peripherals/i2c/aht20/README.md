# AHT20 温湿度传感器示例

该示例通过公共 I2C Driver 驱动 AHT20（7 位地址 `0x38`）：初始化总线后完成
传感器校准，随后每秒触发一次测量，读取并校验 CRC，最后以定点格式
（`xx.xx`）通过串口打印温度（℃）和相对湿度（%RH），共采样 10 次。

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
ecos project create i2c-aht20 --board starrysky-l4-c1
cd i2c-aht20
ecos build
```

使用 StarrySky T1-Pico 时，将板卡参数改为 `--board t1-pico`。

## 连接和运行

ysyx-2512-1 的 I2C0 SCL 复用到 GPIO0[27]、SDA 复用到 GPIO0[28]。AHT20 模块
接至该总线并确认上拉电阻与电平兼容后即可运行。串口输出形如：

```text
[i2c-aht20] AHT20 ready, sampling 10 times
[i2c-aht20] [0] temperature: 25.31 C, humidity: 48.72 %
...
[i2c-aht20] done
```
