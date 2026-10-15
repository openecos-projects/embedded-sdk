# ST7789 QSPI 示例

本示例通过公开的 ST7789 设备 API 初始化开发板的 `display` 资源，驱动屏幕
循环显示红、绿、蓝整屏填充，最后在屏幕中央绘制一个半尺寸的白色窗口，演示
ST7789 的显示窗口（set window）以及行列偏移（horizontal/vertical offset）
处理。

在 StarrySky L4C1 上，显示屏使用 QSPI0 的片选 0，GPIO0[29] 作为 DC，
GPIO0[30] 作为复位，GPIO0[31] 作为背光。这三个引脚在初始化期间配置为输出；
初始化时 ST7789 先进行硬件复位（复位脚拉低 100ms 后再拉高），背光保持
高电平。QSPI 的数据引脚和时钟由 L4 Target HAL 配置。
