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
- 最终产物：ELF、BIN、TXT、HEX
- 默认目标为配置的 firmware name。
- 成功构建仅输出内存使用报告，编译器告警仍正常显示。

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

## 单一来源

以下文件是普通和 isolated 工程的权威来源：

- `board/StarrySkyL4/Makefile`
- `board/StarrySkyL4/Makefile_isolated`
- `components/soc/ysyx-2512/startup/start.S`
- `components/soc/ysyx-2512/linker/sections.lds`
- `components/soc/ysyx-2512/startup/loader.c`

`ysyx-2512` Target 是启动、链接和 SoC HAL 的单一来源。2.x Board 文件仅保留兼容入口；
3.0 工程通过 Board 映射选择 Target，不再复制这些 SoC 文件。

## 已验证基线

`l4test/st7735_clock` 是当前板上基线。它验证了 Flash/PSRAM 启动、SYS UART、GPIO、Timer、QSPI 和 ST7735 全屏颜色循环。
