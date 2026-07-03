# StarrySkyL4 板卡适配说明

## 主要变更

1. `board/StarrySkyL4/board.h`
   - 头文件保护宏从 `__STARRYSKY_L3_H__` 改为 `__STARRYSKY_L4_H__`。
   - 增加 `stdint.h` 引入，保证寄存器宏中的 `uint32_t`/`uint8_t` 类型可见。
   - 将 L3.1 外设基址更新为 L4 MMIO：
     - `UART0`: `0x1000_0000`
     - `RCU`: `0x1000_2000`
     - `RTC`: `0x1000_4000`
     - `WDG`: `0x1000_5000`
     - `ARCHINFO`: `0x1000_6000`
     - `GPIO0`: `0x1010_0000`
     - `GPIO1`: `0x1010_1000`
     - `GPIO2/PINCTRL`: `0x1010_2000`
     - `UART1`: `0x1010_3000`
     - `I2C`: `0x1010_4000`
     - `PS2`: `0x1010_5000`
     - `PWM0`: `0x1010_6000`
     - `PWM1`: `0x1010_7000`
     - `TIM0..TIM3`: `0x1010_8000` 到 `0x1010_B000`
     - `QSPI`: `0x1020_0000`
     - `RNG`: `0x1030_0000`
     - `CRC`: `0x1030_1000`
   - L4 架构图只列出 `PWM0` 和 `PWM1`，未继续保留 L3.1 的 `PWM2` 寄存器宏。

2. `board/StarrySkyL4` 构建配置
   - `Makefile` 的 `CATEGORY` 改为 `StarrySkyL4`。
   - `Makefile` 改为包含 `board/StarrySkyL4/build_conf.mk`。
   - `Makefile_isolated` 的 `CATEGORY` 改为 `StarrySkyL4`。
   - `build_conf.mk` 的驱动扫描路径从 `StarrySkyL3_1` 改为 `StarrySkyL4`。
   - `board.kconfig` 的平台配置从 `STARRYSKY_L3_1` 改为 `STARRYSKY_L4`。
   - `tools/scripts/config.mk` 的 `CATEGORY_LIST` 增加 `StarrySkyL4`，避免 L4 工程执行 `make menuconfig` 时回落到 C2 配置。
   - L4 `MEM` 区域更新为 PSRAM：基址 `0xc0000000`，大小 `8M`；`board.kconfig` 默认内存类型改为 `PSRAM`，大小改为 `8192 KB`。
   - XIP 模式下 `.data/.bss` 仍放在 PSRAM，但 `.stack` 单独放到片上 SRAM：`0x0202_0000 - 0x0202_FFFF`，`_stack_pointer = 0x0203_0000`。这样早期串口打印等非叶子函数不会在 PSRAM 未就绪时破坏返回地址。
   - L4 架构确认为 `rv32im`，普通工程和 isolated 工程的 CFLAGS 改为 `-march=rv32im -mabi=ilp32`。
   - L4 asm 工程的汇编参数改为 `-march=rv32im -mabi=ilp32`，链接 emulation 改为 `elf32lriscv`。

3. 驱动适配
   - `driver/gpio/gpio.c` 增加 `GPIO2` 组的输入、输出、电平、复用配置访问分支。
   - `driver/i2c/i2c.c` 将 I2C 复用管脚改为架构图中的 `GPIO0[27]` 和 `GPIO0[28]`。
   - `driver/hp_uart/hp_uart.c` 将 UART1 复用管脚改为架构图中的 `GPIO0[25]` 和 `GPIO0[26]`。
   - `driver/pwm/pwm.c` 对 `PWM2` 访问增加条件保护；L4 默认无 `PWM2` 寄存器宏时，`timer_id == 2` 返回失败。

4. ECOS 工具入口
   - `bin/ecos-set_board` 增加 `l4` 分支，可将 L4 BSP 注入普通工程。
   - `bin/ecos-set_board_isolated` 增加 `l4` 分支，可将 L4 BSP 注入 isolated 工程。
   - `bin/ecos-set_board_isolated` 的 L4 默认 main 文件来源改为 `templates/hello/l4`。
   - `bin/ecos-init_project` 的模板列表增加 `l4` 列。
   - `bin/ecos-completion.zsh` 的板卡补全增加 `l3_1` 和 `l4`。

5. L4 templates
   - 新增 `templates/*/l4` 子目录，基于对应的 `templates/*/l3_1` 复制生成。
   - `templates/hello/l4/main.c` 的串口输出改为 `hello ysyx, this is l4 board`。
   - `templates/asm_gpio/l4/main.s` 将硬编码 GPIO0 基址从 L3.1 的 `0x1000_2000` 更新为 L4 的 `0x1010_0000`。
   - `templates/asm_hello/l4/Makefile` 和 `templates/asm_gpio/l4/Makefile` 改为生成 `elf32-littleriscv` 目标，并移除未使用的 `../../bin/flash` 目录创建。
   - `templates/smoke_test/l4/main.c` 将 I2C 复用管脚改为 `GPIO0[27]` 和 `GPIO0[28]`，启动日志改为 `StarrySkyL4`。
   - `templates/gpio/l4/main.c` 的启动打印改为 `StarrySkyL4`。

6. 验证工程
   - 在仓库根目录 `l4test` 下维护 L4 测试工程：
     - 通过 `ecos init_project hello -name l4test -target l4` 创建。
     - `main.c` 使用 `hal_sys_uart_init()` 初始化串口。
     - `make alldefconfig` 后生成 `CONFIG_STARRYSKY_L4=y`。
   - 已使用 `rv32im/ilp32` 构建生成 `l4test/hello/build/retrosoc_fw.bin` 和 `l4test/gpio/build/retrosoc_fw.bin`，`app.elf` 确认为 `elf32-littleriscv`。
   - 已确认 XIP 产物中 `_stack_pointer = 0x02030000`，`.data` 运行地址仍为 `0xc0000000`。
   - 已验证 L4 asm hello/gpio 模板均可生成 `elf32-littleriscv` 目标。

## 说明

- 仓库中实际文档目录是 `embedded-sdk/docs`，因此本文档放在该目录下。
- `board/StarrySkyL4/main.c` 仍继承自 L3.1 的 smoke test，仅同步了 L4 的 I2C 管脚和启动打印文本；该 smoke test 中存在历史旧 HAL API 调用，未在本次 MMIO 适配中重写。
- 当前工作区中的 `sdk/toolchain/riscv_unknown` 工具链在本机 glibc 2.35 上无法运行，报缺少 `GLIBC_2.36`/`GLIBC_2.38`；本次最终编译使用 `/home/dallous/Documents/vivado/2025.2/gnu/riscv/lin/bin` 下可运行的 `riscv64-unknown-elf-*` 工具链完成。
