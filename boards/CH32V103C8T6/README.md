# CH32V103C8T6 最小板级适配（实验性）

本适配以 `hello` 工程的编译、链接及产物契约校验为目标，并已在 CH32V103R8T6
上完成控制台实机验收。Board ID 为 `ch32v103c8t6`（别名 `ch32v103-c8t6`），
Target 为 `ch32v103`。
这不是 CH32F103（ARM）适配，也不是其他 CH32V 系列芯片的通用适配。

## 当前范围

- RV32IMAC + Zicsr/Zifencei、ILP32，使用 SDK 锁定的 xPack GCC 15.2.0-1。
- 64 KiB Flash、20 KiB SRAM；Flash 链接至启动别名 `0x00000000`。
- 默认 `flash` profile，保留 2 KiB 栈；提供基本 `.data` 搬运和 `.bss` 清零。
- 仅轮询 USART1：SDK UART port 0 对应 USART1，TX PA9、RX PA10，115200 8N1。
- 使用内部 HSI 8 MHz，不依赖外部晶振，AHB/APB 使用一分频。
- 不提供 GPIO、Timer、PFIC 中断驱动、PLL 切换、USB 或自动下载 provider。
- 未支持 WCH 特有快速中断 ABI；启动路径保持全局中断关闭，异常默认停留。
- 没有声明 `flash` provider，因此 `ecos flash` 会明确报不支持，不能套用 StarrySky 的挂载盘下载流程。

## 创建与编译

先按 SDK 安装指南准备 Python 依赖、CMake、Ninja 和受管交叉工具链。
在开发 checkout 中指定 SDK 路径：

```bash
python3 tools/ecos.py --sdk . project create hello \
  --name hello-ch32v103 --path /path/to/workspace --board ch32v103c8t6
python3 tools/ecos.py --sdk . build \
  --project /path/to/workspace/hello-ch32v103
```

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
| 下载方式 | WCH-LinkUtility，烧录 Intel HEX（数据起始 `0x00000000`） |
| 串口 | USART1 PA9/PA10，115200 8N1，3.3 V 电平，共地 |
| 结果 | 复位后输出 `[I][hello] Hello, World!`，无异常 |

已通过：复位入口、Flash 启动别名、USART1 引脚与波特率、Intel HEX 下载地址。
仍未覆盖：异常/中断行为、HSI 实际频偏、断电重启与长时间稳定性、OpenOCD 烧录路径，
以及 `CH32V103C8T6` 封装实物（当前实机为 CH32V103R8T6）。

## 手动 OpenOCD 烧录（待实机验证）

本次实机验收使用的是 WCH-LinkUtility，OpenOCD 路径尚未在 CH32V103 上实测。
本地 `tools/openocd-wch/` 提供社区 WCH/MRS OpenOCD（该目录被 Git 忽略，
准备方法见 `tools/openocd-wch/ECOS-LOCAL-INSTALL.md`）。前提是 WCH-Link/WCH-LinkE
处于 RISC-V 模式，目标板 SDI 已连接并共地。

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

- `wch-riscv.cfg` 的 flash bank 基址为 `0x00000000`，与 Intel HEX 的地址一致，
  因此优先烧 `.hex`。
- 如果必须烧 `.bin`，地址使用与该 bank 匹配的 `0x00000000`：
  `"$OPENOCD" -f "$CFG" -c "program build/retrosoc_fw.bin 0x00000000 verify reset exit"`。
- 若该适配器上 `reset` 不生效，可改为 `... verify exit`，退出后按板载复位键。
- 若提示 USB 权限不足，按发行包内的 `50-wch.rules` / `60-openocd.rules` 由管理员
  安装 udev 规则；不要直接执行 `bin/start.sh`（它会用 sudo 复制库并修改系统规则）。

判定成功的标准：OpenOCD 输出 `** Programming Finished **` 和 `** Verified OK **`，
并且复位后串口输出 `[I][hello] Hello, World!`。请保留完整 OpenOCD 日志和串口输出；
确认这套命令可用后，再把 OpenOCD 接入 `ecos flash` provider。

## 验证边界

异常/中断行为、HSI 实际频偏、断电重启及长时间稳定性尚未验证；OpenOCD 烧录路径
尚未实机验证；`CH32V103C8T6` 封装也未单独验收。编译成功不能代替这些测试。

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
