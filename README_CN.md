# ECOS 嵌入式 SDK (ECOS Embedded SDK)

**ECOS 嵌入式 SDK** 是专为 StarrySky 系列 RISC-V 芯片及微控制器（如 `StarrySkyC1`, `StarrySkyC2`, `StarrySkyL3`, `StarrySkyL3_1`）打造的生产级、模块化裸机固件开发套件。

本 SDK 围绕全新的 **HAL V2** 硬件抽象层重构，采用 Kconfig 图形化配置与 Make 构建系统，致力于提供高效、安全的嵌入式开发体验。

---

## 目录
1. [核心架构与特性](#核心架构与特性)
2. [工程目录结构](#工程目录结构)
3. [环境依赖与安装](#环境依赖与安装)
4. [标准开发与测试流程 (必读)](#标准开发与测试流程-必读)
5. [HAL V2 API 迁移指南](#hal-v2-api-迁移指南)
6. [第三方组件支持](#第三方组件支持)
7. [致谢](#致谢)

---

### 1. 核心架构与特性
- **HAL V2 硬件抽象层**：摒弃了旧版直接读写底层寄存器（如 `REG_UART_0_RX`）和魔数配置的 Legacy API。采用全套标准化 `hal_*` 接口，支持统一的 GPIO MUX（复用）与 FCFG（功能）配置。
- **Kconfig 模块化构建**：集成 Linux 内核风格的 Kconfig 工具。支持按需裁剪外设驱动（I2C, SPI, RTC 等）和系统组件（libc, libgcc, TimmoLog 等），自动生成配置宏。
- **严格分离的源码树**：提供 `ecos init_project` 工具，支持用户在**操作系统的任意位置**创建独立的项目目录进行开发，杜绝直接在 SDK `templates/` 内修改源码的污染行为。（SDK 开发者可利用专用的 `testdir/` 目录进行内部快速测试验证）
- **XIP (Execute-In-Place) 支持**：支持代码在 Flash 中就地执行，配合 `ld` 链接脚本自动优化内存映射，极大节省 SRAM 空间。
- **丰富的外部设备支持**：原生支持外接外设，如 ST7735 / ST7789 屏幕驱动、PCF8563 外部 RTC 时钟芯片、SGP30 气体传感器等。

---

### 2. 工程目录结构
```text
ecos/embedded-sdk/
├── board/          # 板级支持包 (BSP)，包含各板卡的引脚定义、时钟配置和链接脚本
│   ├── StarrySkyC1/
│   ├── StarrySkyC2/
│   ├── StarrySkyL3/
│   └── StarrySkyL3_1/
├── components/     # 系统级中间件与静态库 (如 libc, libgcc, TimmoLog 日志系统)
├── devices/        # 外部设备驱动组件 (如 st7735, st7789, sgp30, pcf8563)
├── hal/            # 核心硬件抽象层 V2 接口定义 (hal_uart, hal_timer, hal_gpio 等)
├── scripts/        # Make 编译脚本及构建规则
├── templates/      # 官方提供的基础/外设裸机示例模板 (按外设划分，内部再区分具体板卡)
├── testdir/        # (仅供 SDK 开发者使用) 内部专用的快速模板测试与验证环境
└── tools/          # 构建辅助工具 (kconfig, fixdep, ecos 命令行脚手架)
```

---

### 3. 环境依赖与安装

#### 依赖项
- RISC-V 交叉编译工具链 (`riscv64-unknown-elf-gcc` / `riscv32-unknown-elf-gcc`)
- `make`
- `direnv` (推荐，用于 `testdir` 自动加载环境变量)
- Python 3 (可选，用于某些构建脚本自动化)

#### 快速安装
运行根目录下的安装脚本进行基础环境配置：
```bash
./install.sh
```
脚本运行完成后，需要重新打开终端或执行 `source ~/.bashrc` 来更新环境变量。

---

### 4. 标准开发与测试流程 (必读)

> ⚠️ **警告 (CRITICAL RULES)**：
> 绝对禁止直接在 SDK 安装路径下的 `templates/` 目录内修改并执行 `make` 编译！所有的开发与测试必须在 SDK 外部新建的工程目录中进行，以保证原始模板代码不被污染。

#### 步骤 1：创建你的独立工程目录
得益于 `ecos` 命令和环境变量的配置，你可以在电脑的任意位置创建自己的项目。
```bash
# 假设你在自己的开发工作区
cd ~/my_workspace/
```

#### 步骤 2：初始化工程
使用 `ecos` 脚手架工具，从官方模板拉取代码并指定目标板卡（如 `l3_1`）：
```bash
# 格式: ecos init_project <模板名> -name <工程名> -target <板卡型号>
ecos init_project smoke_test -name my_smoke_test -target l3_1
cd my_smoke_test
```

*(注：如果你是参与开发本 SDK 的内核维护者，可直接在 `testdir/` 目录下利用 `direnv` 进行内部模板的快速调试。)*

#### 步骤 3：配置系统 (Kconfig)
在示例工程目录下，你可以选择图形化配置或者无头全默认配置：

- **交互式图形配置 (推荐日常开发者使用)**：
  ```bash
  make menuconfig
  ```
  在界面中勾选你需要的驱动模块、配置优化等级或系统库，保存退出后将生成配置文件。

- **无头默认配置 (推荐 CI/脚本/AI Agent 自动化测试使用)**：
  如果你不需要手动更改菜单，可直接利用底层的 conf 工具静默生成配置：
  ```bash
  $ECOS_SDK_HOME/tools/kconfig/build/conf --alldefconfig Kconfig
  $ECOS_SDK_HOME/tools/kconfig/build/conf --syncconfig Kconfig
  ```

#### 步骤 4：编译固件
```bash
make -j$(nproc)
```
编译成功后，将在 `build/` 目录下生成 `retrosoc_fw.elf` (ELF 调试文件)、`retrosoc_fw.hex` 及 `retrosoc_fw.bin` (二进制烧录文件)，并输出当前 Flash/MEM 内存占用报告。使用 `.bin` 或 `.hex` 文件进行烧录即可。

---

### 5. HAL V2 API 迁移指南
SDK 已全面废弃旧版（C1/C2 时期）的底层寄存器直接操作代码。在编写新代码或迁移老工程时，请严格遵循以下接口映射关系：

- **串口初始化**: `sys_uart_init()` -> `hal_sys_uart_init()`
- **I2C总线**: `i2c_init(...)` -> `hal_i2c_init(...)`
- **定时器与延时**: 废弃硬件死循环 `delay_s() / delay_ms()`，改用基于硬件 Timer 的 `hal_delay_ms(timer_id, ms)`。
- **系统滴答时钟**: 废弃 `sys_tick_init()`，改用标准的 `hal_sys_tick_init(0)` 与 `hal_get_sys_tick(0)`。
- **GPIO 引脚配置**: 不再使用位掩码拼接直接写 `PADDIR` / `PADOUT`，必须使用封装好的 `gpio_hal_set_mux(port, pin, func)` 及 `gpio_hal_set_fcfg(port, pin, mode)` 来设定引脚的功能与复用模式。

---

### 6. 第三方组件支持

#### TimmoLog (智能日志系统)
本 SDK 深度集成了支持 ANSI 色彩的高级日志系统 [TimmoLog](https://github.com/XHTimmo/TimmoLog)，并专门针对 RISC-V Bare-metal (裸机) 环境进行了 `printf` 串口输出映射，非常适合供开发者与 AI 读取结构化日志。

- **启用方式**：在 `make menuconfig` 的 `Build Configuration -> Library Configuration` 中开启 `TimmoLog Support` 选项。
- **代码使用示例**：
  ```c
  #include "log.h"

  int main() {
      hal_sys_uart_init();
      
      // 初始化日志系统，设置最低打印等级为 DEBUG
      log_init(LOG_DEBUG, NULL);
      
      // 打印带有等级前缀、时间和 ANSI 颜色的日志
      log_info("[SYSTEM] 设备启动成功");
      log_warn("检测到异常输入，自动跳过...");
      log_error("硬件初始化失败！");
      
      return 0;
  }
  ```

---

### 7. 致谢
感谢以下开发者对 ECOS 嵌入式 SDK 的代码贡献：

- [XHTimmo](https://github.com/XHTimmo)
- [maksyuki](https://github.com/maksyuki)
- [Krismile233](https://github.com/Krismile233)
- [FINALxxx](https://github.com/FINALxxx)
- 雪泥喵爪
- Ayana nana
- [myyerrol](https://github.com/myyerrol)
