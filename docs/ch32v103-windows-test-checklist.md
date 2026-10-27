# CH32V103 / SDK 3.0.1 Windows 实机测试 Checklist

适配代码基线：`3.0.1` 分支，提交 `adf5293dae197fbcce6c364fe3232ebd4a710c89`。
适用 Board：`ch32v103c8t6`；Target：`ch32v103`。

推送前自动化结果：192 项测试中 191 项通过、1 项跳过；本地 29 个兼容示例构建通过。
适配提交的 [跨平台 CI](https://github.com/openecos-projects/embedded-sdk/actions/runs/40434222189)
也已通过（含 Windows 主机任务）。这些都不是实机测试记录。

> 本文所有实机结果初始均为**未测试**，请在实际测完后填写。
> Linux 编译、寄存器模型、ELF 检查及 CI 通过，不等于 Windows/WCH-Link 或外设实机通过。
> 先前 `hello` 记录来自 **CH32V103R8T6**，不能代替 **CH32V103C8T6** 封装验收。
> `3.0.1` 是开发分支；清单中的 SDK 版本字段目前仍为 `3.0.0`，本轮以 Git 分支和提交号识别代码。

## 1. 测试记录与判定规则

| 项目 | 本轮填写 |
| --- | --- |
| 测试日期 / 测试者 | |
| Windows 版本 / x64 或其他架构 | |
| Python 版本 | |
| `git rev-parse HEAD` | |
| 芯片完整丝印 / 封装 | |
| 开发板型号 / 原理图版本 | |
| 板上供电 / 实测 VDD、VDDA | |
| WCH-Link 型号 / 固件版本 / 驱动 | |
| 串口适配器 / COM 口 / 驱动 | |
| 示波器 / 逻辑分析仪 | |
| 外接模块及地址 / 跳线配置 | |

- `PASS`：满足该子项的实际数据、通信或波形标准，并保留证据。
- `FAIL`：出现错误、死机、异常复位、错误读回、错误波形等；记录日志，勿反复盲目擦写。
- `BLOCKED`：缺模块、缺接线、缺测量设备、示例不覆盖该功能，或需另写测试程序。
- `N/A`：当前版本未实现/不适用于所测封装；不能计入 PASS。
- **编译与实机分开填**。初始化成功不代表完成传输；打印容量不代表完成 Flash 擦写。
- 修改工程测试时，保留修改后的 `main.c`、`ecos-example.yml`、配置及重新生成的产物；区分原样示例与修改版结果。

## 2. 安全和接线准备

- [ ] 核对是 CH32V103，不是 ARM 架构 CH32F103，也不是其他 CH32V 系列。
- [ ] 核对真实封装的引脚引出；HAL 中存在 GPIOA～D 的编号，不代表封装上每个引脚都可用。
- [ ] BOOT0 选择 Flash 启动；确认复位后退出下载/调试暂停状态。
- [ ] 全部设备共地；本清单按 **3.3 V 板卡及 3.3 V 信号**测试。
- [ ] ADC 输入不超过实测 VDDA，不接负电压；不把 5 V 模块输出直接接入。
- [ ] 不将两个推挽输出短接；PA8 蜂鸣器测试需合适的驱动电路，不能直接驱动大电流负载。
- [ ] 备份 MCU Flash、EEPROM 和重要配置。Board 的 `erase: true` 会擦除目标 Flash；自动解除读保护也可能全片擦除。
- [ ] WWDG/IWDG 测试放最后，确保随时能够重新烧录 `hello` 恢复。
- [ ] 区分 **WCH-Link USB**、**USB-UART** 和 **MCU 原生 USB PA11/PA12**；它们不是同一个接口。

### 引脚与实例对照（无重映射）

| 功能 / 公共实例 | MCU 外设与默认引脚 | 备注 |
| --- | --- | --- |
| console / UART port 0 | USART1 TX=PA9，RX=PA10 | TX 接适配器 RX；默认 115200 8N1 |
| UART port 1 | USART2 TX=PA2，RX=PA3 | 需修改测试工程，console 仍用 port 0 |
| UART port 2 | USART3 TX=PB10，RX=PB11 | 需修改测试工程 |
| gpio-demo | PA0 输入，PA1 输出 | PA0 必须有稳定电平，不悬空 |
| ADC instance 0 channel 0 | ADC1 / PA0 | 与 GPIO/EXTI 测试分开接线 |
| I2C instance 0 | I2C1 SCL=PB6，SDA=PB7 | 外接 3.3 V 上拉，例如 4.7 kΩ |
| SPI instance 0 | SPI1 SCK=PA5，MISO=PA6，MOSI=PA7 | Board 默认软件 CS=PA4 |
| SPI instance 1 | SPI2 SCK=PB13，MISO=PB14，MOSI=PB15 | 软件 CS 可选 PB12；需修改测试工程 |
| PWM instance 0 channel 0 | TIM1 CH1 / PA8 | 默认系统时钟 HSI 8 MHz |
| Timer instance 0 | TIM2 / PFIC IRQ44 | PA1 为轮询示例的观察输出，不是 TIM2 硬件通道输出 |
| SysTick | PFIC IRQ12，HCLK/8 | HSI 默认配置下约 1 MHz tick |
| RTC | LSI 自由运行计数器 | 不是日历 RTC，LSI 精度需实测 |
| DMA channel 0～6 | DMA1 Channel 1～7 | 零基编号；不包含 DMA2 |
| USBFS | PA11=D−，PA12=D+ | 当前仅控制器层，无设备枚举协议栈 |

## 3. Windows 更新与环境检查

### 3.1 拉取本次分支

以下使用 PowerShell，先进入你的 SDK checkout 根目录。不要覆盖尚未保存的本地修改。

```powershell
git status --short
git fetch origin
git switch 3.0.1
git pull --ff-only origin 3.0.1
git branch --show-current
git rev-parse HEAD
git merge-base --is-ancestor adf5293 HEAD
# 上一条退出码应为 0，表示包含本轮适配代码。
```

新建 checkout 可用：

```powershell
git clone --branch 3.0.1 https://github.com/openecos-projects/embedded-sdk.git
Set-Location embedded-sdk
```

- [ ] 当前分支为 `3.0.1`，包含 `adf5293`。
- [ ] 记录实际 HEAD；测试期间不要混用不同提交的源码和旧 HEX。

### 3.2 刷新 CLI、工具链和 wlink

首次安装：

```powershell
python --version
python tools/install.py --shell none
```

已安装受管工具链和正确 x86 wlink，仅刷新 CLI/注册 checkout：

```powershell
python tools/install.py --skip-toolchain --skip-wlink --shell none
```

若以前装过默认 x64 wlink，改装本分支默认 x86 资产：

```powershell
python tools/install.py --force --skip-toolchain --shell none
```

如果提示 SDK 同名注册冲突，先核对原注册路径，再决定是否添加
`--replace-registration`；不要删除或改写已发布的 SDK 版本目录。
`--shell none` 不修改 PowerShell 配置，若当前会话找不到 `ecos`，默认安装可用：

```powershell
$env:Path = "$env:LOCALAPPDATA\ECOS\bin;$env:Path"
```

采用自定义 `--host-prefix` 时，请使用该前缀的 `bin`。
后续始终显式选择当前 checkout，避免调用已安装的旧版 SDK：

```powershell
$SDK = (Get-Location).Path
$Work = Join-Path $SDK "testdir\ch32v103-windows"
$Logs = Join-Path $Work "logs"
New-Item -ItemType Directory -Force -Path $Logs | Out-Null

ecos --sdk "$SDK" sdk current --format json
ecos --sdk "$SDK" toolchain status --format json
ecos --sdk "$SDK" board describe ch32v103c8t6 --format json
```

- [ ] Python ≥ 3.9；CLI 和依赖安装正常。
- [ ] 实际 SDK root 是本 checkout，不是旧 release 目录。
- [ ] 工具链为 xPack `15.2.0-1`；CH32 的 ISA/ABI 为 `rv32imac_zicsr_zifencei / ilp32`。
- [ ] 设备管理器能看到 WCH-Link 调试接口和需要的串口；记录 COM 口。
- [ ] Windows x64 默认使用 **x86 wlink 0.1.3**，适配常见 WCH WDM 驱动。

遇到 `ECOS_FLASH_DRIVER_INCOMPATIBLE` 不要立即改绑所有 USB 驱动。可显式使用已准备的
x86 工具：

```powershell
$env:ECOS_WLINK = "C:\tools\wlink-x86\wlink.exe"
# 或每次 flash 添加 --tool C:\tools\wlink-x86\wlink.exe
```

只修改 PATH 不一定覆盖受管 wlink；不要替换串口接口驱动。
更多说明见 [install.md](install.md)。

## 4. Windows 批量编译与单个烧录流程

所有测试工程放在 `testdir/`，不在 SDK 模板/示例目录直接编译；不用交互式 menuconfig。

### 4.1 构建全部 29 个兼容示例

在第 3 节的同一 PowerShell 会话执行。此步骤**只编译，不批量烧录**。
已有同名工程会直接构建；若以前修改过该工程，请先另存或改 `$Work` 为新的测试目录。

```powershell
function Invoke-Ecos {
    & ecos --sdk "$SDK" @args
    if ($LASTEXITCODE -ne 0) { throw "ecos failed: $args (exit $LASTEXITCODE)" }
}

$Examples = @(
    "adc-oneshot", "archinfo-chip-id", "backup-registers", "coroutine-smoke",
    "crc-compute", "dma-memory-copy", "flash-info", "gpio-basic",
    "gpio-interrupt", "hello", "i2c-aht20", "i2c-at24c64", "i2c-pcf8563",
    "i2c-scan", "i2c-sgp30", "letter-shell-smoke", "mini-os", "power-status",
    "pwm-basic", "pwm-buzzer", "rcu-status", "rtc-counter", "spi-loopback",
    "systick-heartbeat", "timer-interrupt", "timer-polling", "usb-controller",
    "wdg-reset", "wwdg-reset"
)

foreach ($Example in $Examples) {
    $Project = Join-Path $Work $Example
    if (!(Test-Path $Project)) {
        Invoke-Ecos project create $Example --board ch32v103c8t6 --path "$Work"
    }
    Invoke-Ecos build --project "$Project" 2>&1 |
        Tee-Object -FilePath (Join-Path $Logs "$Example-build.log")
}
```

- [ ] 29 个工程均在 Windows 成功编译、链接。
- [ ] 无 `implicit declaration of function`、C 编译错误或未定义符号。
- [ ] `hello` 连续执行两次 `build` 均成功；修改测试工程后增量构建也正常。
- [ ] 至少对 `hello` 执行一次 `build --clean` 并重建成功。
- [ ] `build/` 包含 `.elf/.bin/.hex/.map/.txt/.size`、`compile_commands.json`、`artifacts.json`。
- [ ] `artifacts.json` 的 Board/Target/profile 为 `ch32v103c8t6 / ch32v103 / flash`，ELF 为 ELF32，BIN ≤ 65536 字节。
- [ ] 可另建一个带中文和空格的 `testdir/` 子目录，复测工程创建、构建及烧录路径。

### 4.2 单个示例烧录、串口与记录

先从 `hello` 开始，再逐个测试，不要把看门狗固件作为首个程序。
在终端 A 开监视器（`COM5` 换成实际串口），随后在终端 B 烧录或手动按复位键：

```powershell
# 终端 A：先初始化 $SDK/$Work/$Logs，或填入相同的绝对路径。
$Example = "hello"
$Project = Join-Path $Work $Example
$Port = "COM5"
ecos --sdk "$SDK" monitor --project "$Project" --port $Port --baudrate 115200 --timeout 60 2>&1 |
    Tee-Object -FilePath (Join-Path $Logs "$Example-uart.log")
```

```powershell
# 终端 B：独立 PowerShell 会话需重新设置相同的 $SDK/$Work/$Logs。
$Example = "hello"
$Project = Join-Path $Work $Example
ecos --sdk "$SDK" flash --project "$Project" 2>&1 |
    Tee-Object -FilePath (Join-Path $Logs "$Example-flash.log")
Get-FileHash (Join-Path $Project "build\retrosoc_fw.hex") -Algorithm SHA256
```

- [ ] `flash` 正常识别 Probe/Chip 并成功退出，记录 HEX 摘要。
- [ ] 打开监视器后复位能看到日志；一次性输出示例不能靠“烧录后再开串口”判断无输出。
- [ ] 按键复位、断电重启均正常；需要时断开调试器，仅保留板卡供电与串口复测。
- [ ] 同一 COM 口只由一个监视器占用。若 WCH-Link 复合设备在烧录时使串口重连，重新打开监视器后手动复位。

优先使用 Board 的 `ecos flash` / Intel HEX；HEX 数据从启动别名 `0x00000000` 开始。
BIN 不带地址，WCH-LinkUtility 等按物理 Flash 地址操作时用 `0x08000000`。
不要把两种工具地址规则混用。当前不以 OpenOCD 路径作为验收基线。

**交互输入注意**：当前 `ecos monitor` 只读取并显示串口，不转发键盘输入。
测试 `mini-os` 的 `help/ps/spawn` 或其他交互 RX 功能时，先关闭 `ecos monitor`，
改用 PuTTY、Tera Term 等支持双向输入的串口终端（115200 8N1，无硬件流控）。
两个终端不能同时占用同一 COM 口。

## 5. 29 个示例的核对总表

每行分别填 Windows 构建与实机结果（`PASS / FAIL / BLOCKED / N/A`）。
“基础”仅指该示例覆盖的部分，不等于整模块通过。具体扩展项见第 6 节。

| 序号 | 示例 | Windows 构建 | 实机结果 | 原样示例可核对内容 |
| --- | --- | --- | --- | --- |
| 01 | `hello` | [ ] | [ ] | 复位输出 `Hello, World!`，无乱码 |
| 02 | `archinfo-chip-id` | [ ] | [ ] | 输出 system/chip ID；非全 0/全 1，与所测型号核对 |
| 03 | `rcu-status` | [ ] | [ ] | 输出 RCU status，divider=1/control=0；不是 PLL 切换测试 |
| 04 | `gpio-basic` | [ ] | [ ] | PA0 变化后 PA1 跟随，实测电平 |
| 05 | `timer-polling` | [ ] | [ ] | 5 次 `period elapsed` 和 `timer polling completed`，PA1 有翻转 |
| 06 | `timer-interrupt` | [ ] | [ ] | `Timer0 interrupt ready` 后 `ticks` 约每秒递增 |
| 07 | `gpio-interrupt` | [ ] | [ ] | `PA0 interrupt ready` 后稳定边沿使 `edges` 增加 |
| 08 | `systick-heartbeat` | [ ] | [ ] | 约每秒 `tick 1`；当前每次打印后清计数，不是累计序号 |
| 09 | `adc-oneshot` | [ ] | [ ] | PA0 的 0/中点/近 VDDA 输入得到相应 12-bit 读数 |
| 10 | `pwm-basic` | [ ] | [ ] | PA8 约 1 kHz，25%→75%→停止，每阶段约 2 秒 |
| 11 | `spi-loopback` | [ ] | [ ] | PA7 接 PA6，输出 `SPI1 loopback PASS: 4 bytes` |
| 12 | `i2c-scan` | [ ] | [ ] | 有上拉、无从机为 0 设备；有从机显示真实地址，扫描能结束 |
| 13 | `i2c-at24c64` | [ ] | [ ] | 备份后跨页写读，输出 `PASS: 40 bytes verified` |
| 14 | `i2c-aht20` | [ ] | [ ] | 10 次温湿度采样与 `done`，数值符合环境 |
| 15 | `i2c-pcf8563` | [ ] | [ ] | 写入初始时间后读取 10 次，秒数推进，输出 `done` |
| 16 | `i2c-sgp30` | [ ] | [ ] | serial ID + 20 次采样；前约 15 秒默认 IAQ 值不是故障 |
| 17 | `rtc-counter` | [ ] | [ ] | 原样参数不适合快速验收；按 §6.7 修改测试工程，记录闹钟和清理返回值 |
| 18 | `backup-registers` | [ ] | [ ] | `DR1: 0xA55A`；只覆盖即写即读，不覆盖复位保持 |
| 19 | `crc-compute` | [ ] | [ ] | 输出 4 个字的 CRC；另需独立参考比对，重复一致不足以证明算法正确 |
| 20 | `dma-memory-copy` | [ ] | [ ] | 原样仅 `DMA configured`；必须检查完成、剩余数和目标缓冲，见 §6.9 |
| 21 | `flash-info` | [ ] | [ ] | 输出 `65536 bytes, page 1024 bytes`；容量为后端常量，不是擦写/容量探测 PASS |
| 22 | `power-status` | [ ] | [ ] | 输出 power flags；不覆盖 PVD/Wakeup/Sleep/Stop/Standby |
| 23 | `usb-controller` | [ ] | [ ] | 无 MCU USB 数据线时先测初始化和 `USB status`；不能验收枚举 |
| 24 | `coroutine-smoke` | [ ] | [ ] | worker round 1～3 与 `scheduler completed` |
| 25 | `letter-shell-smoke` | [ ] | [ ] | help 输出与 `command parser completed`；非交互 Shell 常驻测试 |
| 26 | `mini-os` | [ ] | [ ] | 心跳、worker、串口 `help/ps/spawn` 可用，连续运行 |
| 27 | `pwm-buzzer` | [ ] | [ ] | 原样时钟常量非 CH32；先按 §6.5 改为 8 MHz，再测音调 |
| 28 | `wdg-reset` | [ ] | [ ] | 最后执行：arming 后不再持续喂狗，发生自动复位循环 |
| 29 | `wwdg-reset` | [ ] | [ ] | 最后执行：`WWDG enabled` 后自动复位循环，约数百毫秒量级 |

缺少外接模块时填 `BLOCKED`；串口只有 ready 日志不能代替设备读写验证。

## 6. 各模块功能验收与扩展项

### 6.1 启动、串口和 USART2/USART3

- [ ] `hello` 按键复位 10 次、断电重启 5 次，均稳定输出，无需调试器先运行一遍程序。
- [ ] `mini-os` 输入 `help`、`ps`、`spawn`，确认 USART1 RX/TX 双向可用。
- [ ] 保存 USART1 TX 波形/波特率，实际应接近 115200 8N1；记录 HSI 频偏。
- [ ] **扩展工程**：通过 `driver-uart` 分别测试 `ECOS_UART_PORT_1`、`ECOS_UART_PORT_2`。
  `hello` 不会自动测试这两个端口，需在复制出来的工程中添加组件和测试代码。
- [ ] USART2 将 PA2(TX) 接 PA3(RX)，USART3 将 PB10(TX) 接 PB11(RX)，分别回环
  `00/55/AA/FF` 和一段文本。按“发一个字节→用 `ecos_uart_try_read()` 有限轮询接收并比对→
  再发下一字节”的顺序测试；不能在没有 RX 缓冲的轮询模式下先连续发送整段再读取。
- [ ] 两端口分别测 9600、115200；不要根据 API 接受任意参数就推断所有波特率都可用。
- [ ] 新增串口测试期间 USART1 console 仍可正常记录结果。

当前公共 UART 新增 port 2，但其他 Target 是否支持该端口由其后端决定；不能根据公共枚举推断硬件资源。

### 6.2 GPIO、EXTI 和 PFIC

- [ ] PA0 用稳定高/低电平驱动，PA1 正确跟随；确认 PA1 不与外部推挽输出短接。
- [ ] `gpio-interrupt` 用低速方波（例如 1～10 Hz）输入 PA0；双边沿计数与已知边沿数相符。
- [ ] PA0 静止且有确定上下拉时，无持续伪中断；机械按键若未消抖不能用于精确计数。
- [ ] **扩展工程**：分别测试上升、下降、双边沿；测试禁用/启用、注销/重注册。
- [ ] **扩展工程**：EXTI5～9、EXTI10～15 各选两个实际引出的引脚，验证共享源分发，不互相丢回调。
- [ ] 同一 EXTI line 由不同 port 重复占用应返回 BUSY；共享组优先级不一致应被拒绝。
- [ ] 阈值升高后屏蔽低优先级，恢复后继续运行；全局中断开关行为正确。
- [ ] 中断进入/返回后主循环、串口与计数均能长期工作；若停机，记录
  `ecos_ch32v103_last_fault` 的 `mcause/mepc/mtval` 及 ELF/MAP。

当前 PFIC 使用普通 RV32 软件现场保存，不测试硬件压栈、快速 ABI 或嵌套中断。

### 6.3 TIM2 与 SysTick

- [ ] `timer-polling` 默认是 **10 ms**，不是 1 秒；5 次翻转结束。日志耗时可能拉长两次 PA1 翻转之间的间隔。
- [ ] `timer-interrupt` 默认是 **1 秒**；60 秒内计数连续推进，测量周期/抖动并记录。
- [ ] `systick-heartbeat` 默认比较值 1000000，HSI 默认配置下约 1 秒；运行至少 60 秒。
- [ ] SysTick 仅首次回调后停止、持续刷屏或多倍/分频错误均记 FAIL；重复出现 `tick 1` 是当前示例预期。
- [ ] **扩展工程**：TIM2 stop 后回调停止；start 后重新计满周期；注销后能重新配置并注册。
- [ ] TIM2 已注册回调时，复用同一 Timer 做 polling delay 或 init/deinit 应返回 BUSY，不能破坏现有中断。
- [ ] **扩展工程**：检查 1 us、10 ms、65536/65537 us、1 s 及长周期边界；长周期允许预分频量子向上取整。
- [ ] **扩展工程**：TIM2 与 SysTick 同时运行，观察两种计数及主循环；不要在中断回调里打印或阻塞。

### 6.4 ADC1

- [ ] `adc-oneshot` 先 PA0 接 GND，再接约 VDDA/2、近 VDDA，分别记录万用表电压和 ADC 原始值。
- [ ] 数值总体接近 0、2048、4095，随输入单调变化；误差按实际电源、参考源、手册要求判定。
- [ ] 不能只根据串口有数字就判通过；禁止 PA0 悬空后用随机数作为正常结果。
- [ ] **扩展工程**：换测封装实际引出的其他外部通道；16/17 内部通道与相应参考量比对。
- [ ] 合法时钟分频 2/4/6/8 分别测试；非法分频应返回参数错误，不应无限卡在校准。

### 6.5 TIM1 PWM 与蜂鸣器

- [ ] PA8 实测 `pwm-basic` 约 1 kHz，25% 和 75% 占空比；stop 后不再出现周期边沿。
- [ ] stop 只保证计数停止，不要求引脚一定变低；如果负载要求关闭低电平，应先设置 0% 等安全状态。
- [ ] **扩展工程**：测试 0%、1%、50%、99%、100%，并在 100%→25% 后确认恢复 PWM。
- [ ] **扩展工程**：period=65536 时 100% 必须保持高，不能因 CCR 溢出变成 0%。
- [ ] **蜂鸣器测试工程修正**：原样 `ECOS_BUZZER_CONFIG_DEFAULT` 以 50 MHz 计时；CH32 为 8 MHz。
  在复制的 `pwm-buzzer/main.c` 中将配置改为以下内容再构建，记录此修改：

```c
const ecos_buzzer_config_t buzzer_config = {
    ECOS_PWM_DEFAULT, ECOS_PWM_CHANNEL_0, 8000000u
};
```

- [ ] 通过 PA8 的合适驱动电路连接无源蜂鸣器；4 kHz 参考音及音阶频率经仪器确认。
  “能发声”不等于音高正确；不要把 50 MHz 常量造成的频偏记成 HSI 本身误差。

### 6.6 SPI1/SPI2 与 I2C1

**SPI**

- [ ] SPI1 PA7→PA6 回环，PA5 有 SCK、PA4 软件 CS 低有效，4 字节匹配。
- [ ] **扩展工程**：多字节、不同模式/位序、较低/较高合法 SCK 下重复回环；测实际 SCK，不只看请求频率。
- [ ] **SPI2 修改版**：复制 `spi-loopback` 工程，将 SPI ID 全部改为 1，配置 CS 为 PB12，
  PB15(MOSI) 接 PB14(MISO)，观察 PB13(SCK)。原样 Board 资源仍指 SPI1，不能只改接线。
- [ ] SPI2 回环匹配且 USART1 日志正常；打印中的 `SPI1` 字样可同步改成 `SPI2`，勿只凭字样判断实例。
- [ ] 测真实外设时按其协议验证；回环通过不代表 ST7735、Flash 或其他设备协议已适配。

**I2C**

- [ ] PB6/PB7 有上拉；无从机时扫描完成且 0 设备，有从机时只出现实际地址。
- [ ] 同一次扫描包含连续 NACK 和真实 ACK，后面的地址仍可成功探测，不能被 AF 状态卡住。
- [ ] 采集 START/address/ACK/data/NACK/STOP；写读组合中有重复 START。
- [ ] **扩展工程**：用可控从机或 EEPROM 分别读取 1、2、3、4 和较长长度，核对最后一个字节的 NACK/STOP 及完整数据。
- [ ] 连续 100 次读写无丢尾字节；断开从机后有明确错误/超时，再接回并重新初始化后可恢复。
- [ ] 不把“无上拉导致 SDA/SCL 卡死”写成正常无设备结果；当前不保证自动 GPIO 九脉冲总线恢复。
- [ ] 记录实际 SCL：部分示例直接用 `ECOS_I2C_CONFIG_DEFAULT` 的 divider=100，8 MHz 下约
  **80 kHz**，并不自动采用 Board 中 divider=80 的 **100 kHz**。需要 100 kHz 时修改测试工程参数并记录。

外接模块：AT24C64 通常 0x50（按 A0～A2），AHT20 0x38，PCF8563 0x51，SGP30 0x58；
以模块实际跳线和规格为准，不同示例可逐一单独接入。

- [ ] AT24C64 测试前备份；原样覆盖 **0x0010～0x0037 共 40 字节**，跨越 32-byte 页边界。
- [ ] PCF8563 原样会重写参考时间；先保存/接受其原时间被覆盖，再检查秒推进。
- [ ] SGP30 前约 15 秒默认 IAQ 值允许；其后还需正常采样/CRC，不能据此校准气体精度。

### 6.7 RTC/LSI 与备份域

**当前 `rtc-counter` 原样示例的限制**：prescaler=50000、闹钟 +1000 ticks 是按旧的高频输入
估算写的。CH32 使用低频 LSI；若 LSI≈40 kHz，等闹钟可能约 **1250 秒**，并且
`ecos_rtc_deinit()` 当前返回 `ECOS_ERR_UNSUPPORTED`。不能因 500 ms 后打印 `0 -> 0`
就断定 RTC 停止，也不能把后续 cleanup 的 UNSUPPORTED 当作闹钟成功证据。

建议只在 `testdir/` 中的 RTC 测试工程调整：

```c
#define RTC_COUNTER_PRESCALER 40000u  /* 仅按 LSI 约 40 kHz 估算，精度仍需实测 */
#define RTC_ALARM_DELTA_TICKS 5u
```

测试程序的 deinit 阶段单独记录返回值；预期当前版本为 UNSUPPORTED，不继续把该返回值
作为致命 panic，或在记录 `alarm triggered` 后停止测试。保留修改的源码。

- [ ] 修改版能初始化，counter 持续增长，约数秒后 ALRF 置位并输出 `alarm triggered`。
- [ ] 对 60 秒/更长时间实测计数并估算 LSI 频率，记录误差；LSI 不是精密 1 Hz 基准。
- [ ] **扩展工程**：counter 在 0xFFFF→0x10000 处读取不跳变、不撕裂。
- [ ] **扩展工程**：不调用 set_counter 的情况下重复 init，不应意外清空 retained counter。
- [ ] 测试前若备份域选了别的 RTC 时钟源，记录 INVALID_STATE；备份完成后才允许显式复位备份域。
- [ ] `backup-registers` 即写即读得到 0xA55A。
- [ ] **保持性修改版**：先读 DR1，再在需要时写入另一个模式；按键复位后确认原值保持。
  原样示例每次都写 0xA55A，不能用于证明复位保持。
- [ ] 有合适 VBAT 电源/电路时才验证主供电断开后的保持；记录 VBAT 接法。RTC、BKP reset 会清除备份内容。

### 6.8 CRC 与芯片标识

- [ ] Chip ID 多次读取稳定，按准确芯片型号/厂商资料核对；本后端读取 0x1FFFF884 的
  32-bit 型号/版本字并扩展到 64-bit 打印，**不是 96-bit 唯一序列号**。
- [ ] CRC 固定输入 4 个字，多次复位得到相同结果。
- [ ] 用独立的软件模型或同型号 WCH 参考固件，对齐多项式、初值、字/位顺序及 xor-out 后比对，保存参考与实际值。
- [ ] CH32 CRC 后端目前忽略公共 config 的 init/xor/mode，使用硬件固定模式；不能把它标成可编程 CRC 模式通过。

### 6.9 DMA1（需要完成标志与数据校验）

原样 `dma-memory-copy` 只配置并启动 DMA，打印 `DMA configured`；它不等待/核对完成。
没有数据证据时只能标“启动接口通过”，不能标“DMA copy PASS”。

可在复制工程的 `ecos_dma_start()` 之后，用下列代码替换原来的 `DMA configured` 日志；
原来的末尾无限循环保留。这是**修改版测试**，重新构建后才烧录：

```c
uint32_t flags = 0u;
uint16_t remaining = 0u;
for (uint32_t poll = 0u; poll < 1000000u; ++poll) {
    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_dma_get_flags(0u, &flags), "poll DMA");
    if ((flags & ECOS_DMA_FLAG_ERROR) != 0u)
        ECOS_PANIC(LOG_TAG, "DMA transfer error: 0x%X", flags);
    if ((flags & ECOS_DMA_FLAG_COMPLETE) != 0u)
        break;
}
if ((flags & ECOS_DMA_FLAG_COMPLETE) == 0u)
    ECOS_PANIC(LOG_TAG, "DMA completion timeout");
ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_dma_get_count(0u, &remaining), "read DMA count");
if (remaining != 0u)
    ECOS_PANIC(LOG_TAG, "DMA remaining: %u", (unsigned)remaining);
for (uint32_t i = 0u; i < 4u; ++i) {
    if (((volatile const uint8_t *)destination)[i] != source[i])
        ECOS_PANIC(LOG_TAG, "DMA mismatch: %u", (unsigned)i);
}
ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_dma_deinit(0u), "release DMA");
(void)ECOS_LOGI(LOG_TAG, "DMA copy PASS: 4 bytes verified");
```

- [ ] 修改版 COMPLETE 置位、ERROR 不置位、remaining=0，destination 为 01/02/03/04。
- [ ] **扩展工程**：DMA 公共 channel 0～6 分别执行，检查互不影响；测试 byte/halfword/word 与地址对齐。
- [ ] 重复测试先 deinit/reinit 或显式恢复缓冲/地址/计数；不能只反复 start 一个已耗尽的计数器。
- [ ] **回调修改版**：config 打开 complete/error，中断组件加入工程；先注册回调再 start，显式开启全局中断。
  回调只置状态，主循环核对数据；不手动抢占 DMA 已分配的 IRQ27～33。
- [ ] COMPLETE 在回调派发时会被后端清除，回调模式不能再要求主循环一定轮询到同一 COMPLETE 标志。
- [ ] ADC/SPI/I2C/UART 集成 DMA 暂无现成接口/示例；这些端到端传输单独记 BLOCKED/N/A，不由内存复制结果代替。

### 6.10 片内 Flash（破坏性扩展，默认跳过）

- [ ] `flash-info` 只验证接口报告 64 KiB / 1 KiB page；报告值是后端常量，不证明真实容量或擦写。
- [ ] **只有完成备份、显式预留擦写页、确认恢复路径后**，才准备专用擦写工程。
- [ ] 专用链接布局必须排除测试页，含 `.text/.rodata/.data` 的 Flash 装载镜像不得覆盖该页。
  例如最后 1 KiB 页：offset=0xFC00，物理地址 0x0800FC00，启动别名 0x0000FC00；
  当前默认链接脚本没有预留此页，不能直接照此地址擦写。
- [ ] `ecos_flash_*` 参数使用 Flash **offset**，不是把 0x0800FC00 作为 offset 传入。
- [ ] 单页擦除后读回全 FF；偶数字节长度、半字对齐写入已知模式后逐字节比对。
- [ ] 覆盖前两个半字、页末半字；检查相邻非测试页摘要/内容不变。
- [ ] 按键复位和断电重启后，读回已写模式保持；读取程序不能启动时再次擦写同一页。
- [ ] 越界、奇地址、奇字节长度被拒绝；0→1 修改必须先擦除，不能当 RAM 写。
- [ ] **不要在从同一 Flash 运行的普通测试程序里调用 `erase_all()`**；本轮不要求全片自擦除验证。

### 6.11 PWR / PVD / 低功耗

- [ ] `power-status` 能输出 flags，只记为“状态读取”，不推断已进入低功耗。
- [ ] **扩展工程**：先验证可用的 EXTI 唤醒，再进行 Sleep WFI/WFE；唤醒后主循环/串口恢复。
- [ ] Stop/Standby 先核对选项字节与板级唤醒源，保证可重新连接/烧录；不要依赖运行中的 TIM2 从 Stop 唤醒。
- [ ] 记录进入前/期间/唤醒后电流和供电条件，尽量排除调试器、LED、串口供电干扰。
- [ ] PVD 门限和 flag 用受控电源/手册验证；按3.3 V测试时不得为模拟欠压而超出器件/模块额定范围。
- [ ] Standby 唤醒可能走复位路径，不把“不从原调用点返回”直接作为故障；按所选模式规格判断。

没有专用唤醒/测流程序时，本组扩展项记 BLOCKED。

### 6.12 USBFS 控制器（不是 USB 设备枚举验收）

- [ ] 首次只接 WCH-Link/USART console，**不要先接 MCU PA11/PA12 的 USB 数据线**。
- [ ] `usb-controller` 初始化后输出 `USB status`，无超时/panic，系统 console 波特率仍正常。
- [ ] 需要调试器时核对：PLL ready、PLL 输入 HSI/2、倍率12、USB 分频 /1；系统仍 HSI。
  这些是配置/状态检查，不能替代48 MHz实测、USB时钟精度及枚举验证。
- [ ] MCU USB 电路、供电/上拉与3.3 V工作条件由原理图独立确认，不把 WCH-Link 枚举当成 MCU USB 枚举。

当前没有端点缓冲区/描述符/标准请求或 CDC/HID/MSC/Host 栈；控制器层保持 USB 中断源屏蔽。
**本轮不要求出现 Windows COM/HID/MSC 设备。** Windows“未知 USB 设备”或设备描述符读取失败
不能据此判定控制器后端通过，也不能当成已有 CDC/HID 的回归；将枚举项填 N/A。
不要仅为消除 Windows 错误提示手动打开 USB 中断/DMA 端点。

### 6.13 IWDG 与 WWDG（最后执行）

- [ ] `wdg-reset`：记录 arming 到下一次启动的实际时间，连续观察至少 3 次自动复位。
  当前 prescaler=32、reload 计数4000；若 LSI≈40 kHz，约3.2秒，仅作为量级参考。
- [ ] `wwdg-reset`：记录 enabled 到下一次启动时间，约数百毫秒量级，连续观察至少3次复位。
- [ ] 使用调试器/WCH资料核对复位原因；IWDG/WWDG 标志分别为 RCC_RSTSCKR bit29/bit30。
  状态寄存器低位、LSI ready 位或 power-status 日志不能代替 watchdog reset flag。
- [ ] **喂狗修改版**：合法周期持续喂 IWDG 不复位，停止喂狗后复位；WWDG 还需验证合法刷新窗口。
- [ ] 当前启用后 deinit 返回 UNSUPPORTED，不能把这记作“成功关闭看门狗”。
- [ ] 结束后烧回 `hello`，冷启动确认无看门狗复位循环，再结束本轮硬件测试。

## 7. 长时间与组合回归

- [ ] `mini-os` 至少运行30分钟，help/ps/spawn正常，worker和心跳无异常停顿；这是协作式调度，不是抢占式RTOS验证。
- [ ] TIM2/SysTick/低速EXTI组合修改版至少运行30分钟，计数持续且无现场破坏。
- [ ] 同一板卡多次烧录不同示例，再冷启动复测 hello/GPIO/TIM2/I2C；检查外设残留状态与备份域配置影响。
- [ ] 记录任何温升、供电压降、串口乱码、丢数据、异常复位与发生时间；不能仅记“最后还活着”。

## 8. 当前不计入已完成验收的范围

以下均不得用其他子项的 PASS 替代：

- C8T6 之外封装的全部引脚、资源映射；在R8上测得的结果需明确标型号。
- CAN、DMA2、TouchKey、I2C2、ADC2、TIM3/TIM4 的完整适配与测试。
- 完整系统 HSE/PLL 时钟树、动态切频后所有外设计时自动更新；CH32 当前未声明 `rcu-pll` 能力。
- UART/SPI/I2C/ADC 集成中断/DMA API，以及 USB CDC/HID/MSC/Host 协议栈。
- 96-bit UID读取、Flash容量寄存器探测；当前 chip ID/容量常量不能代替它们。
- PFIC私有快速中断ABI、硬件压栈、中断嵌套，以及 OpenOCD 烧录路径。

## 9. 回传结果模板

建议先回传最小闭环：Windows构建+烧录、hello、GPIO、TIM2轮询/中断、EXTI、SPI回环、I2C扫描。
然后补 ADC/PWM/RTC 与新增模块；无需等全部外设都备齐才反馈第一轮问题。

```text
代码提交：
Windows/Python：
芯片丝印/板卡：
WCH-Link/驱动/wlink：
COM口/接线：

子项编号/示例：
原样或修改版（附diff）：
Windows构建：PASS / FAIL
实机：PASS / FAIL / BLOCKED / N/A
预期：
实际（含时间、读数、计数、波形）：
复现次数/总次数：
串口日志：
烧录日志、HEX SHA-256、artifacts.json：
故障时mcause/mepc/mtval（若可读）：
```

相关实现与说明：[Board README](../boards/CH32V103C8T6/README.md)、
[中断API](api/interrupt.md)、[CLI](cli.md)、[安装指南](install.md)。
