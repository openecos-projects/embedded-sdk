# I2C 地址扫描示例

该示例使用公共 I2C Driver 扫描标准 7 位地址范围 `0x08..0x77`。对每个地址只发送
写方向地址字节并检查 ACK，随后立即发送 STOP；不会向从设备写入寄存器地址或数据。

创建并构建工程：

```bash
ecos project create i2c-scan --board starrysky-l4
cd i2c-scan
ecos build
```

使用 StartySky T1 时，将板卡参数改为 `--board t1`。ysyx-2512 与 CL1-2512
SoC 适配均使用 I2C0，其中 ysyx-2512 的 SCL/SDA 复用到 GPIO0[27:28]。连接设备时
需要确认总线电压兼容并为 SCL、SDA 提供合适的上拉电阻。运行后，串口会逐项打印
所有返回 ACK 的设备地址，并在结束时打印设备总数。
