# CH32V103C8T6 四路舵机 PWM

本例使用 PWM 实例 1，TIM3 部分重映射。板级默认时钟为 8 MHz，分频 8，
每个 tick 为 1 µs。TIM3 共享 20000 tick 周期，以 50 Hz 同时驱动四个比较输出。

| SDK 通道 | TIM3 通道 | 信号脚（LQFP48 脚号） | 初始高电平 |
| --- | --- | --- | --- |
| 0 | CH1 | PB4（40） | 1000 µs |
| 1 | CH2 | PB5（41） | 1490 µs |
| 2 | CH3 | PB0（18） | 1500 µs |
| 3 | CH4 | PB1（19） | 2000 µs |

用 `ecos project create pwm-servo-four --board ch32v103c8t6` 创建工程，再用
`ecos build --project pwm-servo-four` 构建。将逻辑分析仪的四个输入分别接上表引脚，
地线接板上 GND。每隔 2 秒，PB5 在 1490 和 1510 µs 之间切换。检查更新前后的
连续波形，PB4、PB0、PB1 应维持初始高电平宽度，四路周期都应保持 20 ms。
实际时长会随板上 HSI 频偏变化。

此接线可以同时使用 USART1（PA9/PA10）、USART2（PA2/PA3）、I2C1（PB6/PB7）
和默认 SPI1（PA5/PA6/PA7）。PB4/PB5 与重映射 SPI1 数据线复用，PB5 与 SMBus
告警线复用。PB0/PB1 同时是 ADC_IN8/9 和重映射 TIM1 的 CH2N/CH3N。
引脚选择参见 [WCH CH32V103 数据手册，表 2-1](https://www.wch.cn/downloads/CH32V103DS0_PDF.html)。
