# SDK 自动化测试覆盖范围

本文档整理 SDK 自动化测试应覆盖的主要内容，用于后续设计测试框架、CI 流程和硬件在环测试计划。

## 测试分层原则

SDK 自动化测试建议按“从快到慢、从静态到目标板”的层次建设，避免一开始就把所有测试都绑定到真实硬件。

优先覆盖能够在 CI 中稳定运行的测试，再逐步加入仿真测试和硬件在环测试。

## 第一阶段：基础必测

### 构建测试

- 各芯片和板级默认配置能够编译通过。
- Debug 和 Release 配置能够编译。
- 关键组件开关组合能够编译，例如 libc、drivers、fs、network、shell。
- 示例工程、demo 和 template 工程能够编译。

### 配置系统测试

- Kconfig 依赖关系正确。
- 默认配置能够生成合法 `.config`。
- 互斥选项能够正确生效。
- 组件开启后依赖能够自动满足，或给出明确错误。

### 单元测试

- libc、printf、string、memory 等基础库。
- 容器、链表、ring buffer、队列等通用模块。
- HAL 中无硬件依赖的部分。
- 协议解析、文件系统抽象、设备模型等纯软件逻辑。

### 静态检查

- 编译警告检查，核心模块至少开启常用 warning。
- clang-format 或项目既有代码风格检查。
- shell/python 等脚本 lint。
- 未使用配置、重复宏、非法 include 路径检查。

## 第二阶段：SDK 重点覆盖

### BSP 和 Board 覆盖

- 每个 board 的默认 defconfig 能构建。
- board 初始化代码能够链接。
- linker script、startup、vector table、memory map 不出错。
- 外设驱动启用后不会破坏构建。

### 驱动接口测试

- GPIO、UART、SPI、I2C、Timer、PWM、ADC、Flash 等驱动 API 编译和基础行为。
- 使用 mock 或 fake backend 测试参数校验和状态机。
- 真实硬件测试只覆盖少量关键路径，避免 CI 过慢。

### 启动和链接产物检查

- ELF、map、bin、hex 等产物能够生成。
- section 布局符合预期。
- flash/ram 使用量不超过板级限制。
- 入口符号、vector table、init section 存在。

### 示例程序回归

- hello world。
- GPIO blink。
- UART echo。
- timer tick。
- flash read/write。
- shell/basic console。
- filesystem demo。
- network demo，如果 SDK 支持。

## 第三阶段：进阶测试

### 仿真测试

- 如果支持 QEMU 或自研 simulator，应运行 smoke test。
- 验证启动日志、退出码和串口输出。
- 仿真测试适合放入 CI，因为不依赖物理板。

### 硬件在环测试

- 自动烧录固件。
- 读取串口日志。
- 验证 GPIO、LED、button 等基础交互。
- 验证 watchdog、reset、bootloader 等基础流程。
- 建议放入 nightly 或手动触发流程，不建议每个 PR 全量执行。

### 兼容性和回归测试

- 历史 bug 对应用例。
- 旧 API 兼容性。
- 配置迁移。
- 关键 ABI/API 变更检查。

## 建议优先级

第一阶段优先建立：

- 所有 board/demo 的构建矩阵。
- Kconfig/defconfig 校验。
- 核心纯软件模块单元测试。
- 编译警告检查。
- 产物大小和 section 检查。

第二阶段加入：

- mock 驱动测试。
- QEMU/simulator smoke test。
- 示例程序输出校验。

第三阶段加入：

- 硬件在环测试。
- 烧录、串口、外设真实验证。
- nightly 全量矩阵。

## 当前仓库建议起点

当前分支建议先建立统一的测试入口，覆盖以下最小闭环：

1. `defconfig -> build -> artifact check`
2. libc、Kconfig、demo 构建纳入自动化
3. 后续再补充 mock 驱动测试、仿真测试和硬件在环测试
