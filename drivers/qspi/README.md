# QSPI 驱动

`driver-qspi` 是 SDK 3.0 面向应用和设备驱动的公共 QSPI 接口。应用包含
`ecos/driver/qspi.h`，不直接访问 HAL 或 SoC 寄存器。

- 支持 QSPI 控制器初始化、释放和 CS0..CS3 选择；
- 提供 8/16/32 位以及批量 32 位帧写入接口；
- 提供命令、地址、数据和 dummy 阶段的连续读写接口；
- `clock_divider` 由板级 `qspi-bus` 资源提供，具体时钟公式由 Target HAL 定义；设备的
  片选由设备资源单独选择。

StarrySky L4C1 的 QSPI0 使用 GPIO0[12..16]、GPIO0[24] 的硬件复用，ST7735 的 DC
信号由 `display` 资源映射到 GPIO0[31]。
