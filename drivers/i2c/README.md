# I2C 驱动

`driver-i2c` 是 SDK 3.0 面向应用的公共 I2C 接口。应用包含
`ecos/driver/i2c.h`，不直接访问 HAL、SoC 寄存器或引脚复用配置。

- `ecos_i2c_init()` 配置并启用控制器；
- `ecos_i2c_probe()` 发送地址字节并返回从设备的 ACK/NACK 状态；
- `ecos_i2c_deinit()` 停止并关闭控制器；
- `ecos_i2c_get_instance_count()` 返回当前 Target 提供的控制器数量。

`clock_divider` 表示控制器输入时钟的分频参数。当前 ysyx-2512 Target 支持 I2C0，
分频范围为 `1..256`，并由 SoC HAL 配置 GPIO0[27:28] 的 I2C 复用功能。
