# StarrySkyL4 板级适配说明

本文档描述当前有效的 L4 架构。完整历史和板上排障记录见 `StarrySkyL4_Changes_since_b353023.md`。

## 内存模型

- Flash：`0x30000000`，16 MiB
- PSRAM：`0xC0000000`，8 MiB
- FSBL 在 Flash 执行。
- SSBL、text、rodata、data 在 Flash 保存加载镜像，在 PSRAM 运行。
- bss 位于 PSRAM，由 SSBL 清零。
- L4 普通和 isolated C 工程固定使用 PSRAM执行模型。
- L4 汇编工程使用 `flash.ld`，保持 Flash-only 执行。

## 启动契约

1. `_start` 设置栈并调用 `_first_bootloader`。
2. FSBL 将 SSBL 从 Flash 搬到 PSRAM。
3. SSBL 搬运 text、rodata、data，并清零 bss。
4. 链接脚本保证 `_text_op == main`。
5. SSBL 跳转到 PSRAM 中的 `main`。

bootloader 使用独立的无优化参数编译，不能依赖尚未搬运的 text 或 libc。

## 构建契约

- 架构：`rv32e`
- ABI：`ilp32e`
- 默认优化：`-O0`
- 对象顺序：startup、bootloader、应用/驱动、链接
- 最终产物：ELF、BIN、TXT、HEX、MAP、SIZE 和 `compile_commands.json`
- 产物清单：`build/artifacts.json`，包含架构、ABI、入口、段布局及各文件摘要。
- 默认目标为 `retrosoc_fw`；可由 CMake `FIRMWARE_NAME` 覆盖。
- 构建通过 `ecos build` 调用 CMake/Ninja，编译器由 SDK 工具链状态解析器提供绝对路径。
- `ecos flash` 按 Board 的 `mass-storage` 配置消费产物清单；`ecos monitor` 使用 Board 的
  PySerial 配置。两者均不调用 2.x Shell/Make 业务逻辑。

## 外设地址

- SYS UART：`0x10000000`
- RCU：`0x10002000`
- RTC：`0x10004000`
- WDG：`0x10005000`
- ARCHINFO：`0x10006000`
- GPIO0：`0x10100000`
- GPIO1：`0x10101000`
- GPIO2/PINCTRL：`0x10102000`
- HP UART：`0x10103000`
- I2C：`0x10104000`
- PS2：`0x10105000`
- PWM0/PWM1：`0x10106000` / `0x10107000`
- TIMER0..3：`0x10108000` 到 `0x1010B000`
- QSPI：`0x10200000`
- RNG：`0x10300000`
- CRC：`0x10301000`

地址存在于 `board.h` 不表示对应驱动已在 L4 上验证。目前 SDK 不提供 L4 I2C、PS2、RCU、RTC 和 WDG 板级驱动。

## QSPI 引脚

- GPIO0[12]：SI00/MOSI
- GPIO0[13:15]：SI01..SI03
- GPIO0[16]：NSS0
- GPIO0[24]：SCK
- GPIO0[31]：ST7735/ST7789 DC GPIO

L4 QSPI 使用经板上验证的 FIFO-first MMIO 顺序，驱动对象固定以 `-O0` 编译。

## 板载按键

- Button 0：GPIO1[7]
- Button 1：GPIO1[8]

两个按键默认上拉为高电平，按下时为低电平。SDK 3.0 应用通过
`ecos/bsp/button.h` 使用逻辑 Button 0/1，BSP 将低电平转换为 `BSP_BUTTON_PRESSED`，
将高电平转换为 `BSP_BUTTON_RELEASED`。

## 板载 LED

- LED 0：GPIO1[5]
- LED 1：GPIO1[6]

两个 LED 均为低电平点亮。SDK 3.0 应用通过 `ecos/bsp/led.h` 使用逻辑 LED 0/1，
BSP 将 `BSP_LED_ON` 转换为低电平，将 `BSP_LED_OFF` 转换为高电平；初始化完成后 LED
保持熄灭。

## Timer

ysyx-2512 提供 Timer0 至 Timer3，L4 HAL 向公共 Timer Driver 报告四个可用实例。
通用应用只使用 `ECOS_TIMER_DEFAULT`；需要使用其他实例时，先调用
`ecos_timer_get_instance_count()` 查询当前 Target 的实例数量。L4 HAL 将 25 MHz 输入时钟
预分频为 1 MHz，因此周期和轮询延时均以微秒为单位。当前支持初始化、启动、停止、
到期查询和轮询延时；该 Timer IP 未接出 CNT 读值，`ecos_timer_get_count()` 返回
`ECOS_ERR_UNSUPPORTED`。中断 callback 需要等待 ysyx-2512 中断控制器完成适配。

## 单一来源

以下文件是普通 CMake 工程的权威来源；isolated/AbstractMachine 仍属于兼容适配层：

- `components/soc/ysyx-2512/CMakeLists.txt`
- `components/soc/ysyx-2512/toolchain.cmake`
- `components/soc/ysyx-2512/startup/start.S`
- `components/soc/ysyx-2512/linker/sections.lds`
- `components/soc/ysyx-2512/startup/loader.c`

兼容入口可以继续通过 `CROSS_COMPILE` 覆盖工具链，但 3.0 普通工程不再读取这些
Board Makefile。

`ysyx-2512` Target 是启动、链接和 SoC HAL 的单一来源。2.x Board 文件仅保留兼容入口；
3.0 工程通过 Board 映射选择 Target，不再复制这些 SoC 文件。

## 已验证基线

`l4test/st7735_clock` 是当前板上基线。它验证了 Flash/PSRAM 启动、SYS UART、GPIO、Timer、QSPI 和 ST7735 全屏颜色循环。
