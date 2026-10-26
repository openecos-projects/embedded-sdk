# SPI driver

`driver-spi` 是标准 SPI 全双工、8-bit frame 公共接口。应用通过
`ecos/driver/spi.h` 使用，不直接访问 SoC 寄存器。

当前 CH32V103 实现使用 SPI1：PA5=SCK、PA6=MISO、PA7=MOSI；CS 由 GPIO Driver
软件控制。SPI mode 0～3、MSB/LSB 优先和时钟频率由配置选择。

当前接口只验证轮询单字节收发和软件 CS。DMA、硬件 NSS、16-bit frame、SPI 中断和
多控制器尚未实现。
