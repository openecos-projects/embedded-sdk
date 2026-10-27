# CH32V103C8T6 最小板级适配（实验性）

本适配以 `hello` 工程的编译、链接及产物契约校验为目标，并已在 CH32V103R8T6
上完成控制台实机验收。Board ID 为 `ch32v103c8t6`（别名 `ch32v103-c8t6`），
Target 为 `ch32v103`。
这不是 CH32F103（ARM）适配，也不是其他 CH32V 系列芯片的通用适配。

## 当前范围

- RV32IMAC + Zicsr/Zifencei、ILP32，使用 SDK 锁定的 xPack GCC 15.2.0-1。
- 64 KiB Flash、20 KiB SRAM；Flash 链接至启动别名 `0x00000000`。
- 默认 `flash` profile，保留 2 KiB 栈；提供基本 `.data` 搬运和 `.bss` 清零。
- 轮询 USART：SDK UART port 0 对应 USART1（TX PA9、RX PA10），port 1 对应
  USART2（PA2/PA3），port 2 对应 USART3（PB10/PB11）；默认控制台为 USART1 115200 8N1。
- 提供 GPIOA-D 的 0..15 号引脚基础输入/输出，Board 的 `gpio-demo` 使用 PA0 输入、PA1 输出。
- 提供 TIM2 轮询/中断定时器：固定 HSI/APB1 8 MHz，长周期自动选择预分频量子，
  支持 1～536870912 微秒周期，1 秒周期无需改例程。
- 提供 IWDG 看门狗：使用 LSI，支持合法硬件分频和 12-bit reload；启用后只能复位关闭。
- 提供 I2C1 轮询主机：默认 PB6=SCL、PB7=SDA，标准模式，需外接上拉电阻。
- 提供 SPI1/SPI2 轮询主机：实例 0 为 SPI1（PA5/PA6/PA7），实例 1 为 SPI2
  （PB13/PB14/PB15），依次为 SCK/MISO/MOSI；Board 默认 SPI1，PA4 为软件 CS。
- 提供 TIM1 CH1 PWM 轮询输出，默认 PA8。
- 提供 RTC/LSI 计数器和闹钟比较基础后端。
- 提供 ADC1 单通道轮询采样，支持外部通道 0～15 和内部通道 16/17。
- 提供基础 RCU/HSI 状态、片内 Flash、CRC、备份寄存器、PWR/低功耗、WWDG、DMA1
  channel 1～7 和 USBFS 控制器生命周期接口；DMA 外设请求映射、USB class/descriptor
  栈和真实 Flash 擦写仍需按具体板级连接单独验证。
- 当前新增后端均完成编译/链接验证；PFIC/EXTI/TIM2 另有寄存器模型和 ELF 检查，
  不代表其他后端已通过寄存器模型或实机验证。外设通信、波形、时钟、Flash 擦写和
  低功耗行为仍需按接线单独复测。
- 使用内部 HSI 8 MHz，不依赖外部晶振，AHB/APB 使用一分频；USBFS 控制器初始化会
  配置独立的 HSI/2×12=48 MHz USB PLL 时钟，不切换系统/USART 时钟。
- 提供实验性 PFIC 中断、GPIO EXTI 边沿中断、TIM2 周期回调和 SysTick IRQ12；不依赖
  `WCH-Interrupt-fast`。
- 使用普通 RISC-V ABI 和软件保存现场，关闭 PFIC 硬件压栈/嵌套/VTF，不依赖
  `WCH-Interrupt-fast`；启动保持全局中断关闭，应用显式使能。
- 异常默认记录 `mcause/mepc/mtval` 并停机；无 PLIC。DMA1 有通用通道/回调 API，
  尚未提供 ADC/SPI/I2C/UART 的集成 DMA 传输接口。
- 声明 `wlink` 烧录 provider：`ecos flash` 通过 wlink CLI 烧录 Intel HEX；
  芯片处于读保护时按 Board 配置先执行 `unprotect`（全片擦除）。

## 创建与编译

先按 SDK 安装指南准备 Python 依赖、CMake、Ninja 和受管交叉工具链。
在开发 checkout 中指定 SDK 路径：

```bash
python3 tools/ecos.py --sdk . project create hello \
  --name hello-ch32v103 --path /path/to/workspace --board ch32v103c8t6
python3 tools/ecos.py --sdk . build \
  --project /path/to/workspace/hello-ch32v103
```

如果更新前已安装过全局 `ecos`，请从修复后的 checkout 运行
`python3 tools/install.py --skip-toolchain --skip-wlink --shell none`（Windows 使用
`python`），同步刷新 CLI 内置的工具链清单。仅修改 checkout 的 JSON 不会更新已安装
CLI 的副本；同名 SDK 注册冲突的处理见 `docs/install.md`。

安装并正确选择本开发 SDK 后，也可使用：

```bash
ecos project create hello --board ch32v103c8t6
cd hello
ecos build
```

产物位于工程 `build/`：`retrosoc_fw.elf`、`.bin`、`.hex`、`.map`、`.txt`、`.size`、
`compile_commands.json` 及 `artifacts.json`。应确认 ELF32、ISA/ABI、Flash 大小上限
与清单一致，且编译中无隐式函数声明或链接警告。

`.hex` 为标准 Intel HEX，数据记录从 `0x00000000` 开始，可直接交给 WCH-LinkUtility
等支持 Intel HEX 的工具；`.bin` 不含地址，按物理 Flash 基址 `0x08000000` 写入
（与启动别名 `0x00000000` 等价）。

## 基础 IP 测试

### GPIO 轮询

```bash
ecos project create gpio-basic --board ch32v103c8t6
cd gpio-basic
ecos build
ecos flash
ecos monitor --port <串口设备>
```

将 PA0 接到 3.3 V 或 GND，PA1 接示波器或逻辑分析仪。串口应报告输入电平变化，
PA1 应跟随 PA0。PA0/PA1 不得接入超过 3.3 V 的电平；该测试不覆盖 GPIO 中断。

### TIM2 轮询

```bash
ecos project create timer-polling --board ch32v103c8t6
cd timer-polling
ecos build
ecos flash
ecos monitor --port <串口设备> --expect "timer polling completed"
```

预期出现 5 次 `period elapsed` 和 `timer polling completed`。TIM2 使用 HSI 8 MHz
和 1 MHz tick，当前只验证轮询 update flag，不代表 HSI 频率精度或中断周期精度。

### PFIC、GPIO EXTI 和 TIM2 中断

GPIO 外部中断（PA0 外接测试输入，需稳定上拉/下拉，0～3.3 V）：

```bash
ecos project create gpio-interrupt --board ch32v103c8t6
cd gpio-interrupt
ecos build
# 先开串口，再烧录/手动复位，避免错过 ready 日志
ecos monitor --port <串口设备>
```

在另一终端烧录 `ecos flash --project <工程路径>`，确认 `PA0 interrupt ready` 后，
让 PA0 输入产生至少 10 次上升/下降沿，`edges` 应持续增加。示例没有软件消抖，
不能用机械按键判断精确计数。

TIM2 周期中断（无需外接信号）：

```bash
ecos project create timer-interrupt --board ch32v103c8t6
cd timer-interrupt
ecos build
ecos flash
ecos monitor --port <串口设备>
```

先打开串口再手动复位。应出现 `Timer0 interrupt ready`，随后 `ticks` 约每秒增加，
连续观察至少 60 秒，并检查停止、注销、重新注册与重复启动。
`Timer0` 是公共实例 0，CH32 后端对应 TIM2/PFIC IRQ44。

EXTI0..4 独立 IRQ；5..9、10..15 分别共享 IRQ39/56，同组注册优先级必须相同。
每个编号 EXTI line 只能由一个 GPIO port 占用；不支持电平触发。
优先级 1..16（越大越紧急）、阈值 0 放行全部；不支持中断嵌套或回调中打开全局中断。

本次没有连接实物执行这些步骤：寄存器模拟、向量/现场 ELF 检查和构建通过，不能
据此宣称真实中断已验收。UART/SPI/I2C/ADC 集成中断模式仍未提供；DMA1/SysTick
回调已有后端，但其真实中断行为也需单独上板验证。
详情见 [中断 API](../../docs/api/interrupt.md)。

### I2C1 扫描

```bash
ecos project create i2c-scan --board ch32v103c8t6
cd i2c-scan
ecos build
ecos flash
ecos monitor --port <串口设备>
```

PB6 为 SCL、PB7 为 SDA，外接 3.3 V 上拉。示例扫描 0x08～0x77；没有外设时应得到
0 个设备，不应把总线浮空的结果写成设备通过。

### SPI1 回环

```bash
ecos project create spi-loopback --board ch32v103c8t6
cd spi-loopback
ecos build
ecos flash
ecos monitor --port <串口设备>
```

用短线连接 PA7（MOSI）到 PA6（MISO），PA5 为 SCK、PA4 为软件 CS，保持 3.3 V
电平。收到 `SPI1 loopback PASS` 才算回环通过；该测试不代表外部 Flash 或显示器
设备驱动已经通过。

### SPI1 回环

```bash
ecos project create spi-loopback --board ch32v103c8t6
cd spi-loopback
ecos build
ecos flash
ecos monitor --port <串口设备>
```

用短线连接 PA7（MOSI）到 PA6（MISO），PA5 为 SCK、PA4 为软件 CS，保持 3.3 V
电平。收到 `SPI1 loopback PASS` 才算回环通过；该测试不代表外部 Flash 或显示器
设备驱动已经通过。

### ADC1、PWM 和 RTC

```bash
ecos project create adc-oneshot --board ch32v103c8t6
cd adc-oneshot
ecos build
ecos flash
ecos monitor --port <串口设备>
```

`adc-oneshot` 默认采样 ADC1 channel 0（PA0）。模拟输入必须保持在 0～3.3 V，不能
直接接入数字高电平以外的电压范围；ADC 数值的实际电压换算需要按板上 VDD 校准。

```bash
ecos project create pwm-basic --board ch32v103c8t6
cd pwm-basic
ecos build
ecos flash
ecos monitor --port <串口设备>

ecos project create rtc-counter --board ch32v103c8t6
cd rtc-counter
ecos build
ecos flash
ecos monitor --port <串口设备>
```

PWM 默认使用 PA8/TIM1 CH1，RTC 使用 LSI 计数器。PWM 波形需用示波器确认；RTC 的
LSI 频率和闹钟时间精度尚未校准。

```bash
ecos project create pwm-basic --board ch32v103c8t6
cd pwm-basic
ecos build
ecos flash
ecos monitor --port <串口设备>

ecos project create rtc-counter --board ch32v103c8t6
cd rtc-counter
ecos build
ecos flash
ecos monitor --port <串口设备>
```

PWM 默认使用 PA8/TIM1 CH1，RTC 使用 LSI 计数器。PWM 波形需用示波器确认；RTC 的
LSI 频率和闹钟时间精度尚未校准。

### IWDG 看门狗

```bash
ecos project create wdg-reset --board ch32v103c8t6
cd wdg-reset
ecos build
ecos flash
```

该示例会启用 IWDG，之后芯片预计自动复位。IWDG 启用后不能由软件关闭，请先
准备好串口、烧录和恢复流程；不要在有未保存数据时运行。

## 通过 ecos flash 烧录（wlink）

Board 已声明 `wlink` provider：

```yaml
flash:
  provider: wlink
  artifact: hex
  chip: CH32V103
  unprotect: true
  erase: true
```

`tools/install.py` 会安装 pinned wlink `0.1.3`（Windows 为
`%LOCALAPPDATA%\ECOS\lib\ecos\tools\wlink\`），`ecos flash` 会自动使用它。
Windows 默认受管资产是 x86 wlink，即使主机是 Windows x64 也使用
`wlink-v0.1.3-win-x86.zip`；它兼容常见的 WCH WDM 驱动 `WCHLink_A64`，不需要修改
WCH-Link 设备标识。旧版本已安装 x64 wlink 时，从修复后的 checkout 执行：

```powershell
python tools/install.py --force --skip-toolchain --shell none
```

保留发行包文件，并按 x86 工具要求准备 WCH DLL。构建后任选一种方式覆盖：

```powershell
ecos build
# 1) 每次显式指定（优先于环境变量）
ecos flash --tool C:\tools\wlink-x86\wlink.exe
# 2) 或在当前会话设置环境变量后使用默认命令
$env:ECOS_WLINK = "C:\tools\wlink-x86\wlink.exe"
ecos flash
```

受管安装存在时，仅把其他 wlink 放进 `PATH` 不会覆盖它。
`incompatible driver is installed for this interface` 会返回
`ECOS_FLASH_DRIVER_INCOMPATIBLE`，而不是“设备未找到”；CLI 不会自动换用其他工具或
驱动。若确实需要 x64 wlink，需从 wlink release 手动下载 x64 资产，并使用其他工具
修改 WCH-Link 调试设备标识或绑定 WinUSB；这不是默认支持路径，可能影响
WCH-LinkUtility/MRS。请确认接口后操作，不要替换串口接口驱动。

`unprotect: true` 表示芯片处于读保护时允许先执行 `wlink unprotect`（会全片擦除）
再烧录。若希望遇到保护状态直接失败：

```powershell
ecos flash --no-unprotect
```

成功时文本模式会输出 Probe、Chip 和 SHA-256；如果执行了解除保护，还会提示
`Read protection disabled (chip mass-erased).`。`--format json` 会返回
`provider`、`chip`、`read_protected`、`unprotected`、`tool_output` 等字段。

## 下载与上电检查

`hello` 只在复位后输出一次。请**先打开串口监视器，再复位或重新上电**，否则很容易
错过整段日志。若仍无输出，按以下顺序排查：

1. 确认 BOOT0 跳线处于 Flash 启动位置，而不是 ISP/Bootloader 位置。
2. 使用 3.3 V 电平的串口适配器并与目标板共地；PA9(TX) 接适配器 RX，
   PA10(RX) 接适配器 TX，波特率 115200 8N1。
3. 核对开发板原理图：板载 USB 串口应连接到 USART1 的 PA9/PA10，而不是
   USART2 的 PA2/PA3 或重映射到 PB6/PB7 的 USART1。
4. 确认下载后 MCU 已退出调试暂停并开始运行（WCH-LinkUtility 的复位/运行，
   或手动按复位键）。
5. 用示波器观察 PA9：初始化后 TX 空闲应为高电平，复位后应出现一次约
   115200 8N1 的短突发；若完全没有波形，问题在启动/下载/引脚，而不是日志格式。

## 实机验收记录

| 项目 | 结果 |
| --- | --- |
| 日期 | 2026-10-23 |
| 硬件 | CH32V103R8T6（64 KiB Flash / 20 KiB SRAM） |
| 下载方式 | wlink CLI（`unprotect` + `flash`）及 WCH-LinkUtility，Intel HEX（数据起始 `0x00000000`） |
| 串口 | USART1 PA9/PA10，115200 8N1，3.3 V 电平，共地 |
| 结果 | 复位后输出当前 hello 日志（`Hello, World windows!`），无异常 |

已通过：复位入口、Flash 启动别名、USART1 引脚与波特率、Intel HEX 下载地址、
wlink 解除读保护与烧录。
GPIO/TIM2 固件已覆盖编译、链接和产物回归；PFIC/TIM2/EXTI 增加寄存器模拟和
向量表/保存现场 ELF 检查。PA0/PA1 环回、波形、真实中断进入/返回及周期精度尚未实测。
仍未覆盖：异常/中断行为、HSI 实际频偏、断电重启与长时间稳定性、OpenOCD 烧录路径，
以及 `CH32V103C8T6` 封装实物（当前实机为 CH32V103R8T6）。`ecos flash` 的 wlink
provider 已实现并通过单元测试，待用实物复测。#30 的 Windows x64 驱动错误诊断和
x86 工具覆盖路径有自动化回归测试，但本次修复未进行 Windows/WCH-Link 实机复测。

## 手动 OpenOCD 烧录（在 CH32V103 上实测失败，仅作记录）

> 2026-10-23 实测：社区 WCH OpenOCD 能连接并识别 RISC-V 核，但 `flash probe` 读到
> `device id = 0xffffffff`、`flash size = 65535KB`，`program ... verify` 在
> `0x00000000` offset 0 写失败。该路径不作为本 Board 的烧录方式；请使用
> `ecos flash`（wlink）或 WCH-LinkUtility。以下命令仅作记录。

`tools/openocd-wch/` 提供的是 **Linux x86_64** 社区 WCH/MRS OpenOCD（该目录被
Git 忽略，准备方法见 `tools/openocd-wch/ECOS-LOCAL-INSTALL.md`），不能在 Windows
上运行。前提是 WCH-Link/WCH-LinkE 处于 RISC-V 模式，目标板 SDI 已连接并共地。

```bash
cd /path/to/hello-ch32v103
OPENOCD=/path/to/embedded-sdk/tools/openocd-wch/bin/openocd
CFG=/path/to/embedded-sdk/tools/openocd-wch/bin/wch-riscv.cfg

test -f "$CFG" || { echo "missing $CFG"; exit 1; }
test -f build/retrosoc_fw.hex || { echo "run ecos build first"; exit 1; }

# 1) 工具与配置自检（不接硬件）
"$OPENOCD" --version
"$OPENOCD" -f "$CFG" -c shutdown

# 2) 连接探测（不写 Flash）
"$OPENOCD" -f "$CFG" -c "init; targets; flash banks; shutdown"

# 3) 烧录 + 校验 + 复位
"$OPENOCD" -f "$CFG" -c "program build/retrosoc_fw.hex verify reset exit"
```

Windows 没有随仓库提供 OpenOCD。请使用 MounRiver Studio 自带的
`...\MounRiver_Studio\toolchain\OpenOCD\bin\openocd.exe`，或
Community-PIO-CH32V/tool-openocd-riscv-wch 的 Windows 发行包（含 `openocd.exe`、
DLL 和 `wch-riscv.cfg`）。该仓库的 `main` 分支就是 Windows 包，可直接下载：
`https://github.com/Community-PIO-CH32V/tool-openocd-riscv-wch/archive/refs/heads/main.zip`。
通用 xPack OpenOCD 不带 WCH SDI/`wch_riscv` Flash 驱动，
无法驱动 WCH-Link；WCH-Link 驱动随 WCH-LinkUtility/MRS 安装。Windows 下优先使用
发行包自带的 cfg：

```powershell
cd C:\path\to\hello-ch32v103

$OPENOCD = "C:\path\to\OpenOCD\bin\openocd.exe"
$CFG     = "C:\path\to\OpenOCD\bin\wch-riscv.cfg"

& $OPENOCD --version
& $OPENOCD -f $CFG -c shutdown
& $OPENOCD -f $CFG -c "init; targets; flash banks; shutdown"
& $OPENOCD -f $CFG -c "program build/retrosoc_fw.hex verify reset exit"
```

- `wch-riscv.cfg` 的 flash bank 基址为 `0x00000000`，与 Intel HEX 的地址一致，
  因此优先烧 `.hex`。
- 如果必须烧 `.bin`，地址使用与该 bank 匹配的 `0x00000000`：
  `"$OPENOCD" -f "$CFG" -c "program build/retrosoc_fw.bin 0x00000000 verify reset exit"`。
- 若该适配器上 `reset` 不生效，可改为 `... verify exit`，退出后按板载复位键。
- 若提示 USB 权限不足（Linux），按发行包内的 `50-wch.rules` / `60-openocd.rules`
  由管理员安装 udev 规则；不要直接执行 `bin/start.sh`（它会用 sudo 复制库并修改系统规则）。

判定成功的标准：OpenOCD 输出 `** Programming Finished **` 和 `** Verified OK **`，
并且复位后串口输出 `[I][hello] Hello, World!`。当前 OpenOCD 未接入 `ecos flash`；
本 Board 的烧录 provider 是 `wlink`。

## 验证边界

异常/中断行为、HSI 实际频偏、断电重启及长时间稳定性尚未验证；OpenOCD 路径在
CH32V103 上实测失败，未采用；`CH32V103C8T6` 封装也未单独验收。编译成功不能代替这些测试。

准备实物时，请先核对准确型号与开发板原理图，再按 WCH 官方下载工具/适配的 WCH-Link
流程处理 ELF/HEX/BIN 的地址。BIN 不携带地址，按物理 Flash 基址 `0x08000000` 写入。
使用 3.3 V 兼容串口并共地，PA9 接适配器 RX，PA10 接适配器 TX；
不要将不兼容电平接入 MCU。复位后控制台输出 `Hello, World!` 已在 CH32V103R8T6 上验证。

## 参考与实现边界

寄存器地址、内存布局及启动 CSR 约定核对自 WCH 官方仓库
<https://github.com/openwch/ch32v103>，参考提交：
`ff367d8fec5c9f40029355b2806cd059d9508e4e`。
主要参考 `EVT/EXAM/SRC/Peripheral/inc/ch32v10x.h`、`SRC/Ld/Link.ld` 及
`SRC/Startup/startup_ch32v10x.S`。本适配未引入 WCH 标准外设库或复制其启动实现；
USART 后端和最小启动代码在 ECOS 内独立实现，仍需对照芯片手册和实物验证。
