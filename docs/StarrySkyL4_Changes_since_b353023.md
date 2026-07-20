# StarrySkyL4 完整变更日志

更新时间：2026-07-12

基准提交：`b353023719cac6d8dfe390158eba8f89067d6c4a`（`feat(board): add StarrySkyL4 BSP support`）

本文档覆盖此前同名日志，记录 StarrySkyL4 从基准提交到当前版本的完整有效变更。当前实现以 `verif/` 的启动、链接和 QSPI 行为为基准，并以 `l4test/st7735_clock` 的板上运行结果作为最终验证。

## 当前状态

- 普通 C 工程和 isolated C 工程均从 Flash `0x30000000` 启动，将 SSBL、代码和数据搬运到 PSRAM `0xC0000000` 后执行。
- L4 bootloader、链接脚本和构建顺序已统一为单一镜像方案，不再使用旧的 app/loader 两次独立链接与 payload 嵌入流程。
- `l4test/st7735_clock` 已在板上验证：SYS UART 正常输出，ST7735 可按红、绿、蓝、白、黄、青、品红、黑循环刷新。
- L4 默认优化级别为 `-O0`。QSPI 驱动对象始终以 `-O0` 编译，避免应用选择 `-Os/-O2` 时改变已验证的 MMIO 轮询时序。
- 普通 `make` 默认生成 ELF、BIN、TXT 和 HEX，成功构建时 stdout 只输出最终内存使用报告。

## 构建流程

### 与 verif 对齐

- 使用 `riscv-none-elf-gcc` 将每个 `.c/.S` 单独编译为对象文件。
- 使用 `riscv-none-elf-ld` 和 L4 PSRAM 链接脚本链接最终 ELF。
- 链接顺序固定为：
  1. `start.S`
  2. `loader/loader.c`
  3. 应用、驱动和组件
  4. 最终链接
  5. `objdump` 和 `objcopy`
- bootloader 使用与 `verif` 相同的基础编译参数，不继承应用优化参数，避免复制循环被改写为尚未搬运到 PSRAM 的 `memcpy()` 调用。
- 明确设置 `.DEFAULT_GOAL := $(FIRMWARE_NAME)`，直接执行 `make` 会链接完整固件，不会只生成 bootloader 对象。
- 普通和 isolated Makefile 均使用 `@` 隐藏编译、链接、转换和清理命令；保留编译器真实告警与最终内存报告。

### 工具链与产物

- 默认工具目录为 `/home/dallous/share/project/darch/tools/builder/target/bin`，可通过 Make 变量覆盖。
- 生成以下产物：
  - `build/<firmware>.elf`
  - `build/<firmware>.bin`
  - `build/<firmware>.txt`
  - `build/<firmware>.hex`
- BIN 使用 `objcopy -S -O binary` 生成。
- HEX 使用相对 Flash 基址 `0x30000000` 的地址生成。

## 启动和链接

### 启动入口

- `start.S` 与 `verif/start.S` 保持相同的最小入口：清零 `s0`、设置 `_stack_pointer`、调用 `_first_bootloader`。
- FSBL 位于 Flash，一级 bootloader 将 SSBL 搬运至 PSRAM。
- SSBL 继续搬运 `.text`、`.rodata` 和 `.data`，清零 `.bss`，然后进入 `main`。

### 链接布局

- `.fsbl`：VMA/LMA 均位于 Flash `0x30000000`。
- `.ssbl`：VMA 位于 PSRAM，LMA 位于 Flash。
- `.text/.rodata/.data`：VMA 位于 PSRAM，LMA 位于 Flash。
- `.bss`：位于 PSRAM，类型为 `NOLOAD`，由 SSBL 清零。
- `.data` 完整收集 `.data.*` 和 `.sdata.*`；`.bss` 完整收集 `.bss.*`、`.sbss.*` 和 `COMMON`。
- `.rodata` 同时收集 `.rodata.*`、`.srodata.*` 和 `.eh_frame*`。
- 同时支持 GCC 生成的 `.text.main` 与 `.text.startup.main`。
- 链接期断言 `_text_op == main`。若主入口没有位于 PSRAM text 首地址，链接直接失败，禁止生成会跳入其他函数的固件。
- 当前栈指针位于已链接镜像之后预留的 1 MiB 栈空间顶部。

### 已修复的启动故障

- 修复 `-Os` 将 bootloader 复制循环替换为 `memcpy()`，导致搬运前访问 PSRAM text 的问题。
- 修复 `.text.startup.main` 未被优先放置，bootloader 实际跳入 `timer_delay_ms` 而不是 `main` 的问题。
- 修复 `.data.*` 未被 bootloader 搬运的问题。
- 增加 `.bss` 清零，满足 C 语言静态存储期对象的启动语义。

## QSPI 与 ST7735

### L4 引脚

- QSPI SCK：GPIO0[24]
- QSPI MOSI/SI00：GPIO0[12]
- QSPI SI01..SI03：GPIO0[13:15]
- QSPI NSS0：GPIO0[16]
- ST7735 DC：GPIO0[31]，普通 GPIO 输出

QSPI 引脚复用由 `hal_qspi_init()` 完成，应用不再直接配置 QSPI 引脚寄存器。

### 驱动同步

- 阻塞式等待带超时返回，异常时不再永久死等。
- 所有旧写 API 统一采用板上验证的顺序：
  1. 写 TX FIFO
  2. 写传输长度
  3. 写 START 与 CS
  4. 轮询完整 STATUS 回到 idle
- 新增 `hal_qspi_write_32_repeat()`，用于将同一个 32 位数据循环预填 1 到 32 个 FIFO word；实现与 `verif::qspi_write32_repeat()` 的 MMIO 顺序一致。
- QSPI 对象强制使用 `-O0`，确保轮询和 MMIO 写入顺序不受应用优化等级影响。

### 板上排障记录

排障过程中依次发现并修复：

1. `-Os` 使一级 bootloader 调用未加载的 `memcpy()`。
2. `_text_op` 指向 text 首个辅助函数而不是 `main`。
3. SDK 32-word 和 8-word 批量 API 与 `verif` 的 FIFO/LEN/START 顺序不同，出现超时和屏幕条纹。
4. 强制等待观察 busy 对短传输过严，可能在首次读取前错过完整 busy 窗口。
5. 最终恢复 `verif` 的完整 STATUS 完成判定、`-O0` 编译时序和 FIFO-first 发送顺序。

最终板上串口可持续输出 `color: <name>`，屏幕按预期循环切换纯色。

## 其他 L4 驱动

- GPIO：使用 32 位无符号掩码，支持 GPIO0、GPIO1 和 GPIO2 的输入、输出、电平与复用设置。
- Timer：支持 TIMER0..TIMER3；实现微秒、毫秒、秒延时以及系统 tick API；默认 Timer 时钟为 25 MHz。
- HP UART：使用 GPIO0[25:26] alternate-0，分频值基于配置的 CPU 时钟计算。
- PWM：PWM0 使用 GPIO1[14:17]，PWM1 使用 GPIO1[18:21]；不再访问不存在的 PWM2。
- SYS UART：保持与 `verif` 相同的 16550 初始化和轮询发送流程。
- ARCHINFO：修复 `uint32_t` 与 `%x` 的可变参数类型告警。
- 删除未在 L4 上验证的 I2C、PS2、RCU、RTC 和 WDG 板级驱动及对应 L4 模板。

## 内存报告

- 使用当前构建选择的交叉 `objdump`，不再依赖宿主机 `objdump`。
- Flash 使用量包含 `.fsbl/.ssbl/.text/.rodata/.data` 的完整加载镜像。
- PSRAM 使用量包含 `.ssbl/.text/.rodata/.data/.bss` 的运行镜像。
- L4 报告显示各 PSRAM 段明细，不再错误显示 `PSRAM 0 B`。

## 项目生成与配置

- `ecos init_project` 直接调用同一 SDK 目录中的 `ecos-set_board`，避免 PATH 指向旧 SDK 时混用文件。
- isolated 项目同样直接调用当前 SDK 的 `ecos-set_board_isolated`。
- `set_board l4` 会复制 board 目录中的 Makefile、链接脚本、启动文件和 bootloader。
- 普通与 isolated 项目在覆盖 loader 目录前先删除旧目录，避免残留旧的 `payload.S/loader.lds/start.s`。
- Kconfig 同步只删除临时的 `include/generated` 和 `include/config`，不会再删除用户项目的整个 `include/` 目录。
- L4 默认优化改为 `-O0`，其他板卡仍默认 `-Os`。
- 全部 L4 C 模板的 `start.S` 和 `sections.lds` 与 `board/StarrySkyL4` 单一来源保持一致。
- L4 汇编模板继续使用 Flash-only 链接脚本和 `elf32lriscv`。
- `ecos init_project list` 不再宣称支持已经移除的 L4 外设模板。

## 验证

### 板上验证

验证工程：`l4test/st7735_clock`

```sh
make defconfig
make clean
make
```

已确认：

- Flash bootloader 正常进入 PSRAM `main`。
- SYS UART 初始化和持续输出正常。
- QSPI 与 ST7735 初始化正常。
- 128x128 屏幕可持续进行 8 色全屏循环。

### 静态检查

- bootloader 不引用 PSRAM 中尚未搬运的 `memcpy()`。
- `_text_op` 与 `main` 地址相等。
- ELF 的 Flash LMA 与 PSRAM VMA 符合链接契约。
- QSPI repeat API 的反汇编顺序为 FIFO 循环、LEN、START、STATUS 轮询。
- 普通 `make` stdout 只包含最终内存使用表。

## 兼容性说明

- L4 非汇编工程固定使用 PSRAM执行模型。Kconfig 中 XIP 选项仅为旧配置兼容保留，不改变当前链接行为。
- QSPI 新增 repeat API 当前由 StarrySkyL4 驱动实现；跨板使用前需要对应 BSP 提供实现。
- 修改 board 目录的启动或链接文件后，应同步全部 `templates/*/l4`，并重新验证普通与 isolated 项目生成流程。
