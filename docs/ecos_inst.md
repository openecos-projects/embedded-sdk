# ECOS 指令工具链解析 (ecos_inst)

本文档详细解析了 ECOS 嵌入式 SDK 中 `bin/` 目录下的核心指令脚本的实现原理及使用方法。这些脚本基于 Bash 编写，为开发者提供了便捷的工程管理和板级配置功能。

## 1. 总入口脚本: `ecos`

### 1.1 文件位置
[`bin/ecos`](file:///home/timmo/Documents/ecos/embedded-sdk/bin/ecos)

### 1.2 功能与原理
`ecos` 是整个命令行工具链的主入口，主要职责是**命令分发 (Command Dispatching)** 和 **全局环境变量推导**。
- **路径解析**: 脚本首先利用 `BASH_SOURCE` 解析出自身的绝对路径，进而推导出 SDK 根目录 `SDK_ROOT`。
- **日志与错误处理**: 封装了 `log()`, `warn()`, `err()` 方法，提供了统一的 ANSI 颜色输出。
- **动态分发机制**: 它将传入的第一个参数作为子命令（如 `init_project` 或 `set_board`），然后拼接为对应的子脚本路径（`ecos-<command>`）。如果该文件存在，则通过 `exec` 替换当前进程直接执行子脚本，并将后续参数传递过去。
- **内置帮助**: 若没有输入参数或输入 `help`，则打印出可用的子命令及其说明。

## 2. 工程初始化脚本: `ecos-init_project`

### 2.1 文件位置
[`bin/ecos-init_project`](file:///home/timmo/Documents/ecos/embedded-sdk/bin/ecos-init_project)

### 2.2 功能与原理
该脚本用于从 `templates/` 目录复制示例代码，初始化一个全新的开发工程。

- **模板列表 (`ecos init_project list`)**: 
  内置了 `echo_available_templates` 函数，以 Markdown 表格的形式打印出当前 SDK 支持的工程模板及其适配的硬件板型号（如 `hello`, `gpio`, `minesweeper` 等对 `c1`, `c2`, `l3` 的支持情况）。
- **参数解析**:
  脚本可以接收多个参数：`<template_name>`，可选的 `-name <new_project_name>` 以及可选的 `-target <target_board>`。如果未提供目标板型号，默认使用 `c1`。
- **模板拷贝与特殊处理**:
  - 对于 C 语言工程，直接从 `templates/<template_name>/<target_board>` 拷贝到目标文件夹。
  - 对于汇编语言工程（`asm_hello`, `asm_gpio`），除了拷贝文件外，还会在目标目录下生成特殊的 `.ecos-project` 配置文件，指明 `TYPE=asm`。
- **项目级配置**:
  - 在新工程下生成 `.ecos-project`，记录 `BOARD=<target_board>`。
  - **联动机制**: 在拷贝完成后，脚本会自动调用 `ecos set_board <target_board>` 来将对应板卡的 BSP 文件（Makefile, board.h, 链接脚本等）注入到新工程中。
  - 此外，还会创建 `configs` 目录，并将 `tools/scripts/` 下的构建规则文件复制到工程内。

## 3. 板级配置脚本: `ecos-set_board`

### 3.1 文件位置
[`bin/ecos-set_board`](file:///home/timmo/Documents/ecos/embedded-sdk/bin/ecos-set_board)

### 3.2 功能与原理
该脚本用于将特定的硬件开发板 (Board Support Package) 配置环境注入到当前的工程目录中。支持 `c1`, `c2`, `l3`。

- **上下文校验**:
  `check_project_dir()` 函数检查当前目录是否包含 `Makefile` 或 `.ecos-project`，以防止在非 ECOS 工程目录下误操作。
- **汇编工程特殊处理**:
  如果 `.ecos-project` 中标记了 `TYPE=asm`，脚本只会拷贝 `board/<target_board>/Makefile_asm` 为当前的 `Makefile`，不再拷贝 C 语言工程需要的 C 头文件。
- **文件注入**:
  对于常规工程，根据目标板型号，脚本会从 `board/<target_board>/` 中提取以下核心文件复制到当前工程根目录：
  - `board.h`：板级硬件引脚与宏定义头文件。
  - `Makefile`：针对该板卡的工程编译脚本。
  - `sections.lds`：对应硬件内存布局的链接脚本。
  - `start.s` / `start.S`：对应硬件特性的中断向量表与汇编启动代码（如 C2 是 `start.S`）。
- **配置持久化**:
  成功注入 BSP 文件后，脚本会自动更新 `.ecos-project` 文件中的 `BOARD=<board_name>`，保证环境记录与实际文件一致。

## 4. 固件烧录脚本: `ecos-flash`

### 4.1 文件位置
[`bin/ecos-flash`](file:///home/timmo/Documents/ecos/embedded-sdk/bin/ecos-flash)

### 4.2 功能与原理
该脚本用于将编译好的二进制固件自动烧录（拷贝）到开发板中。

- **上下文校验**: 
  同样调用 `check_project_dir()` 检查当前目录是否为有效的 ECOS 工程。
- **设备挂载检测**: 
  检查目标挂载目录 `/run/media/$USER/YSYX-HFPLnk` 是否存在。该路径为特定开发板或系统的默认挂载点。
- **固件搜索与拷贝**:
  自动兼容两种不同编译结构生成的产物路径：
  - 根目录下的 `retrosoc_fw.bin`（如汇编工程直接生成）。
  - `build/` 目录下的 `build/retrosoc_fw.bin`（如基于 Makefile 框架编译的 C 工程）。
  使用静默拷贝并添加 `sleep 0.5` 防止硬件缓冲未就绪的问题。如果找不到固件，则会提示用户先执行 `make` 编译。

## 5. 串口监视器脚本: `ecos-monitor`

### 5.1 文件位置
[`bin/ecos-monitor`](file:///home/timmo/Documents/ecos/embedded-sdk/bin/ecos-monitor)

### 5.2 功能与原理
该脚本封装了对 `minicom` 串口工具的调用，为开发者提供了一键打开串口监视器的功能。

- **前置依赖检查**:
  首先检查系统中是否已安装 `minicom`，如果未安装则会输出错误并提示用户进行安装。
- **智能串口识别与多选交互**:
  - 脚本会自动扫描 `/dev/ttyUSB*` 和 `/dev/ttyACM*`。
  - 如果只发现一个设备，自动绑定该设备。
  - **如果发现多个串口设备**，它会弹出一个交互式的选择菜单（基于 bash 的 `select` 语法），让用户输入序号来选择正确的板卡串口，从而避免连接错误设备。
  - 也可以直接通过传参手动指定，例如：`ecos monitor /dev/ttyUSB1`。
- **快速执行**:
  最终直接调用 `exec minicom -D <PORT> -b 115200`，以项目默认的 115200 波特率打开串口。

## 总结
通过这三个脚本的设计，ECOS SDK 实现了一种高内聚、低耦合的项目管理模式：
- `ecos` 作为统一网关。
- `init_project` 负责应用层代码（模板）的提供。
- `set_board` 负责底层硬件配置文件的热插拔。
开发者只需要一个指令就能搭建起包含对应硬件驱动、编译规则和应用代码的完整工程体系。