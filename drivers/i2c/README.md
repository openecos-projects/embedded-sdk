# I2C 驱动

`driver-i2c` 是 SDK 3.0 面向应用的公共 I2C 接口。应用包含
`ecos/driver/i2c.h`，不直接访问 HAL、SoC 寄存器或引脚复用配置。

- `ecos_i2c_init()` 配置并启用控制器；
- `ecos_i2c_probe()` 发送地址字节并返回从设备的 ACK/NACK 状态；
- `ecos_i2c_write()` 向从设备写入一段连续字节；
- `ecos_i2c_read()` 从从设备读取一段连续字节；
- `ecos_i2c_write_read()` 写入后发出重复 START，再读取一段连续字节，适合设备寄存器访问；
- `ecos_i2c_deinit()` 停止并关闭控制器；
- `ecos_i2c_get_instance_count()` 返回当前 Target 提供的控制器数量。

地址参数使用标准 7 位从设备地址，不要预先左移。写、读和组合事务要求长度大于零，
并在发生 NACK（`ECOS_ERR_NOT_FOUND`）、仲裁丢失（`ECOS_ERR_IO`）或超时
（`ECOS_ERR_TIMEOUT`）时返回错误；地址探测仍使用 `probe()` 的 `1=ACK`、
`0=NACK` 返回约定。

`clock_divider` 表示控制器输入时钟的分频参数，具体寄存器语义由 Target HAL
负责。当前 ysyx-2512 与 CL1-2512 Target 均支持 I2C0；ysyx-2512 的分频范围为
`1..256`，并由 SoC HAL 配置 GPIO0[27:28] 的 I2C 复用功能。

ysyx-2512 的写事务按“START+地址、数据字节、STOP”执行；组合事务在写入后保持总线，
通过重复 START 切换到读地址，最后一个读字节带 STOP。所有轮询阶段均有超时保护，异常
路径会尝试释放总线并关闭未完成的事务。
