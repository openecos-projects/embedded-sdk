# I2C 地址扫描示例

该示例使用公共 I2C Driver 扫描标准 7 位地址范围 `0x08..0x77`。对每个地址只发送
写方向地址字节并检查 ACK，随后立即发送 STOP；不会向从设备写入寄存器地址或数据。

## 支持板卡

| 板卡 | `--board` 参数 | 支持状态 | 控制器 |
| --- | --- | --- | --- |
| StarrySky L4 | `starrysky-l4` 或 `l4` | 支持 | ysyx-2512 I2C0 |
| StartySky T1-Pico | `startysky-t1-pico` 或 `t1-pico` | 支持 | CL1-2512 I2C0 |

当前示例要求板卡提供 `console`、`i2c-bus` 资源，并要求对应 Target 支持 I2C。
以上两块板卡均已声明 I2C0 总线，并提供相应的 Console BSP 和 I2C HAL。

## 创建和构建

创建并构建工程：

```bash
ecos project create i2c-scan --board starrysky-l4
cd i2c-scan
ecos build
```

使用 StartySky T1-Pico 时，将板卡参数改为 `--board t1-pico`。

## 连接和运行

ysyx-2512 与 CL1-2512 SoC 适配均使用 I2C0，其中 ysyx-2512 的 SCL/SDA 复用到
GPIO0[27:28]。连接设备时需要确认总线电压兼容并为 SCL、SDA 提供合适的上拉电阻。
运行后，串口会逐项打印所有返回 ACK 的设备地址，并在结束时打印设备总数。
