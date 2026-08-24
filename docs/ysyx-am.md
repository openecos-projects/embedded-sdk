# StarrySkyL4 ysyx-am 使用说明

## 创建工程

先加载 SDK 和工具链路径，再创建专用工程：

```sh
eval "$(/path/to/embedded-sdk/bin/ecos env)"

ecos init_project ysyx-am \
    -name am_l4 \
    -target l4 \
    --core rv32e-base
```

`--core` 必须指定 SDK 中已登记的核心 profile。am-kernels 已随 SDK 同步到
`ysyx/am-kernels`，不需要安装、选择或配置外部路径。`ysyx-am` 不能与
`-isolated` 混用。

工程的 `configs/ysyx-am.mk` 记录以下路径：

- `ECOS_SDK_HOME`：当前 embedded-sdk 根目录。
- `AM_HOME`：SDK 内迁入的 `ysyx/abstract-machine`。

程序源码路径由工程 Makefile 固定为 `$(ECOS_SDK_HOME)/ysyx/am-kernels`，不接受
工程配置、环境变量或命令行覆盖。

SDK 内的 AbstractMachine 是面向 StarrySkyL4 的精简副本，只保留
`riscv32e-ysyxsoc`、L4 ysyxsoc 启动代码、AM/Klib 公共接口和 RV32E 所需的软算术
实现；其他 ISA、仿真器平台和旧设备实现不随 SDK 分发。

SDK 移动后，可以修改 `ECOS_SDK_HOME` 或重新创建工程。执行
`ecos set_board l4` 只刷新 ysyx-am 元数据，不会覆盖工程根 Makefile。

## 选择核心

StarrySkyL4 是拼片 SoC。当前 RTL `core_wrapper.sv` 不足以确认最终
`core_sel -> RTL_CORE_ID` 映射，因此当前登记两个构建 profile。`rv32e-base`：

- `-march=rv32e -mabi=ilp32e`；
- 不执行 CSR 指令，不开放 CTE、yield 或 thread-os；
- 不启用 M/A 扩展；
- `halt()` 使用稳定循环，不使用 EBREAK。

`rv32e-zicsr` 是候选 profile，使用 `-march=rv32e_zicsr -mabi=ilp32e`，仅声明
Zicsr 编译能力；它尚未绑定物理 `core_sel`，也没有完成 ECALL/MRET、RTL 仿真和
板测，因此仍不开放 CTE、yield 或 thread-os。

运行 `make cores` 查看登记项。开放 Zicsr/CTE 程序前必须提交物理 core_sel 映射、
GPR/CSR/ECALL/MRET/EBREAK RTL 审计、与 AM trap 序列一致的仿真结果和实际板测
记录。只证明汇编器接受 `-march=..._zicsr` 不构成硬件支持证据。

## 选择和构建程序

```sh
make
make list
make list-all
make cores
make APP=kernels/hello
make APP=benchmarks/microbench MAINARGS=test
make APP=benchmarks/coremark
make APP=benchmarks/dhrystone
make hello
make APP=tests/cpu-tests TEST=add
make clean
```

不带参数的 `make` 只显示板卡、核心能力、设备能力和当前可构建程序，不启动
编译器。`manifest.mk` 是唯一程序授权源；未知路径、路径穿越、NEMU-only 程序、
状态为 `conditional` 的程序以及缺少核心或设备能力的程序都会在编译前失败。
`cpu-tests` 必须按 `TEST` 子项逐个审核；当前尚未授权任何 L4 CPU 测试项。

当前按“完整编译、链接并生成可加载 ELF”标准开放 `kernels/hello`、
`benchmarks/microbench`、`benchmarks/coremark` 和 `benchmarks/dhrystone`。
这一级别不代表 benchmark 已在真实板卡上完成结果或分数验证。

microbench 可通过 `MAINARGS` 选择 `test`、`train`、`ref` 或 `huge` 规模。首次
验证推荐使用：

```sh
make APP=benchmarks/microbench MAINARGS=test
```

未指定 `MAINARGS` 时仍传入空字符串，microbench 会按自身规则选择 `ref`。其他
程序不接受非空 `MAINARGS`，非法规模也会在编译前失败。

构建会把已授权程序复制到当前工程的 `build/staging/<program-id>/`，对象和库写入
`build/work/<program-id>/`，最终产物写入：

```text
build/images/<program-id>/retrosoc_fw.elf
build/images/<program-id>/retrosoc_fw.bin
build/images/<program-id>/retrosoc_fw.txt
build/images/<program-id>/retrosoc_fw.hex
```

SDK 内置 am-kernels 和 SDK 不产生构建文件。`make clean` 只删除当前工程的 `build/`。
烧录方式沿用 StarrySkyL4 的 Flash 工具，入口位于 `0x30000000`；FSBL 将 SSBL、
text、rodata 和 data 搬到 `0xc0000000` 开始的 8 MiB PSRAM。

## 当前能力

| 能力 | 状态 | 说明 |
| --- | --- | --- |
| UART TX/RX | 构建支持 | 使用 SYS UART HAL，RX 为非阻塞轮询 |
| Timer uptime | 已验证可用 | Timer0 32 位微秒计数，软件累计一次回绕；用于三个已开放 benchmark |
| Flash read | 实现、未开放清单能力 | 使用 QSPI HAL，地址和片选仍需板测 |
| RTC | 不支持 | 计数起点和日历语义未确认 |
| PS2/Input | 不支持 | FIFO、清除语义和扫描码状态机未验证 |
| GPU | 不支持 | 屏幕型号、分辨率、方向、GPIO 和 QSPI CS 未确认 |
| CTE | base profile 不支持 | 等待逐核心 CSR/ECALL/MRET 仿真与板测 |
| Audio/Disk/Net/VME | 不支持 | 当前没有完整 HAL 或内存契约 |
| MPE | 单核 | `cpu_count()=1`，不宣称多核支持 |

Timer uptime 必须至少在每个完整 32 位回绕周期内读取一次，否则软件无法判断多次
回绕。长期计时需要后续增加中断或 HAL 64 位时基。microbench、coremark 和
dhrystone 均通过该 Timer0 uptime 获取运行时间。

PSRAM 总容量为 8 MiB，链接镜像和 1 MiB 栈之后的空间才是可用 heap。microbench
会跳过 heap 不足的单项；因此默认 `ref` 和 `huge` 规模可能只运行其中一部分，
首次功能验证应使用 `MAINARGS=test`。

## 扩展清单和板卡

程序完成 build、boot 或 hardware 验证后，在
`ysyx/programs/manifest.mk` 中更新状态、所需 core/device features、验证等级和
失败原因。不要只根据目录存在或一次链接成功标记 `supported`。

新增板卡需要提供独立 board profile、startup/loader/linker、HAL 能力表和程序
集合，再扩展初始化器的 target 校验。不能直接复用 L4 内存布局或宣称 L4 清单
对其他板卡有效。
