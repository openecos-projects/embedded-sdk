# 历史板卡迁移状态

本文记录旧版 `Makefile` 工程迁移到 SDK 3.0 Target、Board、Component 和 Example 构建图的状态。
当前 CLI 已同时发现 4 块现有 3.0 板卡和 4 块历史板卡，共 8 个 Board。
下表也列出这一轮补迁外设的现有 T1-Pico 板卡。

## 板卡与 Target

| Board | Target | ISA / ABI | 3.0 profile | 已迁移能力 |
| --- | --- | --- | --- | --- |
| `starrysky-c1` | `ysyx-c1` | `rv32im` / `ilp32` | `flash-xip` | UART、GPIO、I2C、QSPI、Timer、ST7735 |
| `starrysky-c2` | `ysyx-c2` | `rv32im` / `ilp32` | `flash-xip`、`psram-data` | UART、HP-UART、GPIO、I2C、PWM、QSPI、Timer、Archinfo、ST7735、ST7789 |
| `starrysky-l3` | `ysyx-l3` | `rv64ifd_zifencei` / `lp64` | `flash-xip`、`mem` | UART、GPIO、I2C |
| `starrysky-l3-1` | `ysyx-l3-1` | `rv64ifd_zifencei` / `lp64` | `flash-xip`、`mem` | UART、HP-UART、GPIO、I2C、PWM、QSPI、Timer、Archinfo、RNG、CRC、RCU、RTC、PS2、WDG、ST7735、ST7789 |
| `starrysky-t1-pico` | `cl1-2512` | `rv32imc_zicsr_zifencei` / `ilp32` | `flash-xip` | UART、GPIO、GPIO 中断、PLIC、CLINT、I2C、QSPI、Timer、Timer 中断、SDRAM、RCU、RCU-PLL、Heap、LLM Chat |

L3/L3.1 的 profile 会写入 Board 声明的 Kconfig 预设。选择 `mem` 时，配置生成
`CONFIG_LINK_TARGET_MEM=y`，CMake 选择 MEM 链接脚本，代码和数据链接到 `0x80000000`；默认
`flash-xip` 将代码放在 Flash、可写数据放在 MEM。`mem` 构建会先生成应用 `app.elf/app.bin`，
再把 payload 链接进 Flash 中的 loader，最终 `firmware` artifact 可直接交给旧版 loader
启动路径使用。

C2 的 `psram-data` profile 保持代码在 Flash，把 `.data` 和 `.bss` 链接到 `0x40000000`
的 8 MiB PSRAM。启动汇编先使用片内 SRAM 栈，按旧工程的 QSPI/PSRAM 寄存器序列初始化，
再切换到 PSRAM 栈并复制 `.data`、清零 `.bss`。默认 `flash-xip` 继续使用 `0x30000000`
的 128 KiB SRAM。

## 历史 Example 矩阵

这些 Example 已使用当前公共 Driver/BSP API 重新解析并映射到历史 Board：

| Board | 可创建 Example |
| --- | --- |
| `starrysky-c1` | `coroutine-smoke`、`display-donut`、`flash-fatfs`、`flash-sfud`、`hello`、`i2c-aht20`、`i2c-at24c64`、`i2c-pcf8563`、`i2c-scan`、`i2c-sgp30`、`letter-shell-smoke`、`spi-master-st7735`、`text-editor` |
| `starrysky-c2` | `archinfo-chip-id`、`coroutine-smoke`、`display-donut`、`fcc`、`flash-fatfs`、`flash-sfud`、`hello`、`i2c-aht20`、`i2c-at24c64`、`i2c-pcf8563`、`i2c-scan`、`i2c-sgp30`、`letter-shell-smoke`、`pwm-basic`、`pwm-buzzer`、`spi-master-st7735`、`spi-master-st7789`、`text-editor`、`uart-esp01s` |
| `starrysky-l3` | `coroutine-smoke`、`hello`、`i2c-scan`、`letter-shell-smoke` |
| `starrysky-l3-1` | `archinfo-chip-id`、`blink`、`coroutine-smoke`、`crc-compute`、`display-donut`、`flash-fatfs`、`flash-sfud`、`hello`、`i2c-aht20`、`i2c-at24c64`、`i2c-pcf8563`、`i2c-scan`、`i2c-sgp30`、`letter-shell-smoke`、`ps2-keycodes`、`pwm-basic`、`pwm-buzzer`、`rcu-status`、`rng-random`、`rtc-counter`、`spi-master-st7735`、`spi-master-st7789`、`text-editor`、`uart-esp01s`、`wdg-reset` |
| `starrysky-l4-c1` | `archinfo-chip-id`、`blink`、`button-state`、`coroutine-smoke`、`crc-compute`、`display-donut`、`flash-fatfs`、`flash-sfud`、`gpio-basic`、`gpio-tm1650`、`hello`、`i2c-aht20`、`i2c-at24c64`、`i2c-pcf8563`、`i2c-scan`、`i2c-sgp30`、`letter-shell-smoke`、`pwm-basic`、`pwm-buzzer`、`rcu-status`、`rng-random`、`rtc-counter`、`spi-master-st7735`、`spi-master-st7789`、`text-editor`、`uart-esp01s` |
| `starrysky-l4-c2` | `archinfo-chip-id`、`blink`、`button-state`、`coroutine-smoke`、`crc-compute`、`display-donut`、`flash-fatfs`、`flash-sfud`、`gpio-basic`、`gpio-tm1650`、`hello`、`letter-shell-smoke`、`pwm-basic`、`pwm-buzzer`、`rcu-status`、`rng-random`、`rtc-counter`、`spi-master-st7735`、`spi-master-st7789`、`text-editor`、`uart-esp01s` |
| `starrysky-l4-c3` | `archinfo-chip-id`、`blink`、`button-state`、`coroutine-smoke`、`crc-compute`、`display-donut`、`flash-fatfs`、`flash-sfud`、`gpio-basic`、`gpio-tm1650`、`hello`、`letter-shell-smoke`、`pwm-basic`、`pwm-buzzer`、`rcu-status`、`rng-random`、`rtc-counter`、`spi-master-st7735`、`spi-master-st7789`、`text-editor`、`uart-esp01s` |
| `starrysky-t1-pico` | `blink`、`button-state`、`clint-clock`、`coroutine-smoke`、`flash-fatfs`、`flash-sfud`、`gpio-basic`、`gpio-interrupt`、`hello`、`i2c-aht20`、`i2c-at24c64`、`i2c-pcf8563`、`i2c-scan`、`i2c-sgp30`、`letter-shell-smoke`、`llm-chat`、`plic`、`rcu-pll`、`rcu-status`、`sdram-init`、`timer-interrupt` |

以下命令在仓库根目录运行；当前 Python 环境需先安装 CLI 依赖
（`python3 -m pip install ./tools/ecos_cli`），并准备好 SDK 工具链。
CLI 会根据 Example 的 `requires`、Board resources 和 Target capabilities 自动得出这张矩阵：

```bash
python3 tools/ecos.py --sdk . project list --format json
```

## 验证结果

- 正式示例已迁入 `examples/`；本机 Linux x64 使用锁定工具链重新构建当时全部 8 块 Board 的
  149 个 CLI 声明兼容的 Board/Example 组合，逐个核对 artifact 中的 Board、ISA、ABI
  和 ELF32/ELF64 类别。可用
  `python3 tools/ci_host_smoke.py --skip-install --skip-tests --all-examples` 重跑；
  [CI 工作流](../.github/workflows/ci.yml)会在 Linux x64 执行同一矩阵，
  并在另外四类宿主执行安装、测试及含空格和非 ASCII 路径的最小构建。
  各 runner 上传包含 Board、Example、ISA/ABI、工具链、BIN 大小上限和
  ELF/BIN 摘要的 JSON 报告。每次构建要求 BIN 不超过 Target 声明的 Flash 容量。
  CI 另在 Ubuntu x64 的 Python 3.9 环境执行安装、测试和最小构建，验证声明的
  Python 最低版本。
- 此后新增 C2 专用 `fcc` 和 `soft-spi-jedec-id` 示例；当前 CLI 声明 151 个 Example/Board 组合。
- 原有 149 个组合已通过 CLI 完整构建，并生成 ELF、BIN、HEX、MAP、SIZE 和
  `compile_commands.json`；C1/C2 产出 ELF32，L3/L3.1 产出 ELF64。
- `fcc` 的 C2 `flash-xip`/`psram-data` profile 和 `soft-spi-jedec-id` 的 C2 `flash-xip`
  profile 已完成定向交叉构建；这些新增组合尚未计入原有 149 个全矩阵记录。
- L3 和 L3.1 的 `mem` profile 也分别完成了配置、应用 payload 生成、loader 链接和最终制品
  校验；最终 firmware 的入口位于 Flash，payload 位于 MEM 加载地址。
- C2 的 `psram-data` profile 已完成配置与链接检查：Flash 入口为 `0x00000000`，
  `.data/.bss` 位于 PSRAM，GPIO 输出影子值保留启动序列设置的 bit15。
- ST7735/ST7789 使用各自的 Board 资源；C1/C2/L3.1 的新增显示组合与 3 块 L4
  板卡的 ST7789 组合均通过交叉构建。
- L3.1 和 L4C1 的 `hello`、`blink`、`display-donut` 六个阶段 B 组合通过 CLI 构建；
  donut 使用 Board Display BSP、ST7735 Device Driver 与 QSPI Driver 的公共链路。
- `llm-chat` 只保留在 T1-Pico 的支持矩阵中；CI 使用一字节的
  `stories260k_q8.bin`、`tok512.bin` 临时占位文件验证 `.incbin` 与链接，
  不代表真实模型或板上推理已通过。
- `flash-sfud` 已迁移为公共 `sfud` 组件，使用公开 QSPI HAL 完成 Flash 初始化和识别；
  当前构建验证只覆盖编译、链接和产物契约，真实 JEDEC 识别、擦写、校验及掉电恢复仍需实板验收。
- `flash-fatfs` 已迁移为公共 `fatfs` 组件，磁盘适配按 SFUD 设备容量和擦除粒度提供扇区信息，
  挂载失败不会自动格式化；当前构建验证覆盖 7 块带 QSPI Board，真实卷格式化、文件读写和掉电恢复
  仍需实板验收，组件与示例继续从正式 release 排除。
- `letter-shell-smoke` 已迁移为公共 `letter-shell` 组件，使用 3.0 BSP console 适配串口收发；
  当前构建验证只覆盖命令表和有限命令解析，真实串口交互和长时间稳定性仍需实板验收，组件与示例继续从正式 release 排除。
- `text-editor` 已迁移为依赖 FatFs 和 Letter-Shell 的公共组件，示例在自己的命令表中提供 `edit`；
  C1/C2/L3.1/L4 C1/C2/C3 共 6 个组合通过交叉构建。`ram-64k` Target 能力限制静态编辑缓冲
  所需内存，T1-Pico 不支持。C1/C2 的简单 UART 无数据哨兵处理有宿主回归覆盖；文件保存和
  终端交互仍需实板验收，组件与示例继续从正式 release 排除。
- `letter-shell-fs` 已迁移 `cd`、`ls` 和 `touch` 到独立 FatFs 组件；`touch` 不截断已有文件，
  `ls` 逐项输出。宿主测试覆盖目录切换失败后的路径恢复，编辑器示例在 C1/C2/L3.1/
  L4 C1/C2/C3 六个兼容板卡上完成新增命令的交叉构建；实际卷操作仍需实板验收。
- 旧 `spi_software` 已替换为通用 `soft-spi` 组件，使用 `ecos_gpio_pin_t` 和显式 SPI mode，
  不再固定 GPIO0[0:3]。Board 的 `soft-spi-bus` 资源生成公共引脚宏；宿主测试覆盖引脚冲突、
  状态机以及 mode 0-3 收发，C2 提供 `soft-spi-jedec-id` 构建示例，仍需按人工清单完成
  实板接线、外设识别和 mode 0-3 验证。
- `fcc` 已迁移到 C2 开发清单图，生成代码由链接脚本预留在片内可执行 SRAM；
  `flash-xip` 和 `psram-data` profile 均完成定向交叉构建与 ELF 段地址检查。
  宿主测试覆盖参数、全局变量、前向调用、返回语句生成和容量边界；生成代码尚未在实板执行，
  组件与示例继续从正式 release 排除。
- `coroutine-smoke` 已迁移为公共 `light-coroutine` 组件，在无外设依赖的有限轮次中覆盖任务创建、
  上下文保存、让出和任务结束；组件配置可通过 `COMPONENT_COROUTINE_*` Kconfig 选项调整。
- CLI 回归测试覆盖 C2 PSRAM、L3.1 PS2/WDG 的实际构建，以及清单、安装器和资源解析；
  最新通过数量以测试命令输出为准。

单个组合的创建和构建方式：

```bash
mkdir -p /tmp/ecos-work
python3 tools/ecos.py --sdk . project create i2c-scan \
  --name c1-i2c-scan --path /tmp/ecos-work --board starrysky-c1

python3 tools/ecos.py --sdk . build \
  --project /tmp/ecos-work/c1-i2c-scan --format json
```

## 迁移边界

- 历史板卡的寄存器定义已归档到对应 `components/soc/*/include`，`boards/*/board.h`
  只保留给 2.x Makefile 的兼容转发；3.0 Target/HAL 不再反向包含板卡头文件。
- 源码树中的旧板卡 Makefile 仍作为迁移输入保留，但 `tools/sdk-manifest.json` 已将板卡根
  `Makefile*` 和 `build_conf.mk` 排除出正式 release；发布包的核心构建入口只有 CMake/Ninja。

- 显示资源以旧模板为依据：C1/C2 的 ST7735 DC 为 GPIO0[2]；C2 的 ST7789
  DC/RESET 为 GPIO0[0:1]；L3.1 的 ST7735 时钟模板和 ST7789 使用 GPIO1[6] 作为
  DC，ST7789 RESET 为 GPIO1[7]。没有软件控制的复位/背光引脚以固定接线表示；
  L3.1 另一个旧 ST7735 模板使用 GPIO0[31]，需要按实际接线选择板级资源。
- C2 的 PSRAM 初始化以及 PSRAM 与屏幕共享 QSPI 的运行行为尚未在实板上验证。
- L3.1 LED 使用旧 GPIO 模板的 GPIO0[26:27]、高电平点亮，仍需确认实板接线；
  `display-donut` 的画面、刷新速度和两块板的显示接线仍需实板验收。
- WDG 的 `0x5` 控制字和 PS2 的 `0x76` Escape 键码沿用旧例程；它们的实板复位和
  键盘输入行为尚未验收。
- 上述构建验证覆盖清单解析、依赖解析、编译、链接和制品契约；实际板上启动、外设电气连接和
  屏幕变体仍需要接入对应硬件后单独验收。
