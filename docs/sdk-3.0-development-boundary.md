# ECOS SDK 3.0 开发边界

本文定义 ECOS Embedded SDK 3.0 的产品定位、架构边界、迁移范围和验收条件。
它用于约束 `3.0-dev` 上的设计和代码评审，避免在重构过程中继续扩大目标或把
ESP-IDF 的复杂度整体复制到 ECOS。

状态：SDK 3.0 开发基线。

文中的“必须”“禁止”是 3.0 发布要求；“建议”允许在评审后采用等价实现。

## 1. 产品定位

ECOS SDK 3.0 是面向 StarrySky 和兼容 RISC-V 平台的轻量、模块化裸机 SDK。
它借鉴 ESP-IDF 的 Target、SoC、Driver、BSP、Component 和 Example 职责分离，
但不以兼容 ESP-IDF API、构建系统或组件生态为目标。

3.0 必须继续满足以下约束：

- 裸机和静态链接是默认运行模型。
- 不依赖 RTOS 也能完成配置、构建、烧录和监视。
- 默认驱动可以使用同步、轮询和超时接口，不为尚不存在的需求强制引入异步框架。
- SDK 可以支持 RV32、RV64 以及不同 ABI，但目标差异不能泄漏到普通 Example。
- 普通工程引用路径解析器选定的 SDK；该 SDK 可以是正式安装版本或已注册的开发源码，
  工程不得复制 SDK、BSP 或工具链源码。
- SDK 3.0 新工程和 SDK 自有组件统一使用 CMake 描述构建，Ninja 是默认构建执行器。
- Windows、GNU/Linux 和 macOS 使用同一来源、同一版本和同一配置的 RISC-V 工具链；
  不把宿主平台差异变成 Target 或应用源码差异。
- SDK 自有的主机侧 CLI、安装、配置和工具链管理统一使用 Python 实现；正式流程不要求
  Windows 用户安装 Bash 或一组 Unix 命令。

## 2. 3.0 必须解决的问题

### 2.1 单一 Example 源码

同一功能 Example 必须只维护一份应用源码。板卡差异由 Target/SoC、Driver 和 BSP
处理，禁止继续增加 `templates/<example>/<board>/main.c` 形式的应用副本。

“单一源码”不表示所有板卡必须支持所有 Example。构建系统必须根据能力和资源需求
明确接受或拒绝目标板卡。

### 2.2 Target 与 Board 分离

- Target 是构建系统选择的 SoC ID，描述 CPU、工具链、ABI、启动机制和基础构建规则。
- Target ID 直接对应 `components/soc/<target>/`，不再设置独立的 Target 到 SoC 映射层。
- Board 描述具体开发板使用的 Target、内存布局、引脚连接、板载器件和默认配置。
- 工程中的 `board` 和 `target` 使用互相覆盖的选择语义：设置 Board 时必须用 Board
  清单中的映射覆盖 Target；直接设置 Target 时必须清空 Board。
- `ecos project set-board` 面向使用完整 BSP 的常规工程，`ecos project set-target`
  面向只选择 SoC、尚未绑定具体开发板的工程。

### 2.3 稳定的应用接口

普通应用只允许依赖以下公开接口：

- 公共 Driver API，例如 GPIO、UART、Timer、I2C、SPI/QSPI。
- BSP API，例如 Console、LED、Button、Display 和板载存储。
- 公共 Component 和 Device Driver API。

应用不得直接依赖 SoC 寄存器、板卡 `board.h`、PinMux 魔数或内部 HAL/LL 接口。

### 2.4 可验证的依赖和能力解析

构建开始前必须完成以下校验：

- Example 要求的能力是否由 Board 提供。
- Board 要求的外设控制器是否由 Target 对应的 SoC 提供。
- Device Driver 的总线和依赖是否完整。
- 所选 CPU、ISA、ABI、启动文件和链接布局是否一致。

不支持的组合必须在编译前给出可操作的错误信息，不能静默回退到其他板卡配置。

### 2.5 AI 和自动化友好

AI、CI 和普通开发者必须使用同一套公开接口完成能力发现、工程创建、配置、校验、
构建和诊断。AI 友好不以 CLI 命令数量衡量，而以接口是否可发现、可组合、可预测、
可验证和可安全重放衡量。

完成标准不是“AI 能阅读 Shell 脚本后猜出用法”，而是自动化工具仅根据本地帮助、
正式 schema 和机器可读命令输出，就能选择兼容的 Board 和 Example、创建工程、完成
构建，并在失败后获得明确的修复方向。

官方 VS Code 扩展也必须使用同一套 CLI 和 schema 契约，不能成为独立于 CLI、CMake
和清单之外的第四套工程模型。

### 2.6 统一 CMake 构建

SDK 3.0 必须用统一的 CMake target 图表达 Target、BSP、Driver、Component 和
Application 的编译与链接关系。Board 差异必须来自清单和解析结果，不能继续以板卡
Makefile 作为隐藏的构建事实来源。正式核心构建以 Ninja 执行，并在不安装 GNU Make
的环境中工作。

## 3. 明确不属于 3.0 的目标

以下事项不是 3.0 发布的必要条件：

- 兼容 ESP-IDF API、项目格式或二进制组件。
- 引入 FreeRTOS 或把 RTOS 作为默认运行环境。
- 建立在线组件注册表、账号体系或远程依赖服务。
- 为所有驱动实现 DMA、异步回调、线程安全和热插拔。
- 为每个内部函数建立运行时虚函数表或动态驱动模型。
- 保证现有所有板卡支持相同的外设能力。
- 在 3.0 首个版本中迁移全部历史 Example。
- 将预编译固件继续作为 Example 源码的一部分维护。
- 以增加命令数量、自然语言聊天入口或绑定特定 AI 厂商作为“AI 友好”的完成标志。
- 重写 AbstractMachine 等第三方项目自带的全部构建系统；第三方适配器与 SDK 核心
  CMake 构建的边界必须单独声明。
- 为 C/C++ 自研语言服务器、代码索引器或调试适配器；3.0 优先复用编辑器现有能力、
  `compile_commands.json`、GDB 和标准调试接口。
- 在 VS Code Web 中执行本地编译、烧录或串口监视；首发扩展以桌面版和远程扩展宿主
  中能够调用 ECOS CLI 的环境为边界。

构建工具迁移本身不代表架构完成。CMake 必须建立在 Target、Board、Component 和
Example 职责已经分离的基础上，禁止把现有板卡 Makefile 逐个等价翻译后继续维护重复
的源码列表、编译参数和依赖解析。

## 4. 术语和职责

### 4.1 Target

Target 是构建系统公开的芯片选择 ID，直接对应一个 SoC 实现目录。例如
`target: ysyx-2512` 对应 `components/soc/ysyx-2512/`。ECOS 不在 Board 和 SoC 之间
增加另一套具有不同 ID 的软件目标层。Target 负责选择：

- CPU 架构、ISA、ABI 和工具链。
- SoC 能力和外设控制器实例。
- 启动、异常、中断和系统初始化机制。
- 通用链接规则和可用内存类型。
- 目标专用 HAL/LL 实现选择。

Target 不描述开发板上的 LED、屏幕、传感器或具体接线。

### 4.2 SoC

SoC 组件位于 `components/soc/<target>/`，并以 `ecos-soc.yml` 声明 Target 身份。目录保存
寄存器、基地址、中断号、外设信号、芯片能力以及 Target 所需的 CPU/ISA/ABI 和基础构建
信息。能力应能表达外设数量、通道数、FIFO 大小、地址宽度等约束，而不只是
`true/false`。

`ysyx-2512` 的寄存器定义、启动入口、PSRAM loader、链接脚本和 SoC HAL 必须以
`components/soc/ysyx-2512/` 为单一构建来源。StarrySky L4 Board 只负责资源与默认配置，
不得再被 3.0 构建链当作 SoC 实现目录。

### 4.3 LL 和 HAL

- LL 负责寄存器字段、位操作和最底层硬件访问。
- HAL 负责把外设操作过程抽象为相对统一的步骤。

3.0 强制逻辑边界，不强制每个简单外设立即拆成独立 LL 文件。只有出现两个以上
SoC 实现、复杂寄存器流程或可复用价值时，才建议物理拆分 LL。HAL 和 LL 均属于
SDK 内部接口，不承诺跨小版本稳定。

### 4.4 Driver

Driver 是应用可使用的稳定外设接口，负责：

- 端口/实例和资源生命周期。
- 配置结构体、参数校验和统一错误码。
- 超时、中断、并发或 DMA 等已实现能力。
- 调用对应 Target 的 HAL。

Driver API 不得暴露某块开发板的引脚选择。新增 API 应返回统一错误码，避免新增
无法报告失败的 `void` 初始化接口。

### 4.5 Device Driver

Device Driver 描述 ST7735、SGP30、PCF8563 等外部器件。它通过公共 Driver 操作
GPIO、I2C、SPI/QSPI 等总线，并通过配置结构体接收总线实例和控制引脚。

Device Driver 不得根据 Board 名称选择引脚，也不得直接包含具体 BSP 的头文件。

### 4.6 BSP

BSP 描述开发板及板载资源，负责：

- 绑定 Board 到一个 Target。
- 内存容量、启动 profile 和烧录方式。
- PinMux、时钟源和板级电源控制。
- 板载设备使用的控制器、地址、片选和 GPIO。
- 提供 Console、LED、Button、Display 等逻辑资源。
- 声明能力和默认配置。

BSP 可以依赖 Device Driver 和公共 Driver。普通 Example 不得绕过 BSP 重新配置
已经由 BSP 管理的板载资源。

### 4.7 Component

Component 是可独立声明源码、公开头文件、配置和依赖的功能单元。组件依赖必须是
有向无环关系，构建系统只加入项目实际需要的组件及其递归依赖。

3.0 首版只要求支持本地 SDK 组件，不要求在线下载或语义化版本求解。

### 4.8 Example、Template、Test 和 Artifact

- Example：完整、可构建、用于展示功能的应用，允许用户复制后修改。
- Template：带占位符或固定结构的内部脚手架，用于生成 Project 或 BSP。
- Test：具有明确断言、退出状态或结构化通过/失败结果的自动验证程序。
- Artifact：由特定版本、Target、Board 和配置生成的 ELF/BIN/HEX 等产物。

Example 源码放入 `examples/`，真正的生成骨架放入 `templates/`，自动测试放入
`tests/`。Artifact 不得和 Example 源码一起长期提交，正式产物应通过 Release 或
制品系统发布。

## 5. 依赖方向

允许的主要依赖方向如下：

```text
Example/Application
    |---> BSP public API
    |        |---> Device Driver
    |        `---> Public Driver
    |---> Device Driver
    `---> Public Driver
                 `---> HAL
                          `---> LL / SoC registers

Build System ---> Target + Board + Component manifests
```

禁止以下反向依赖：

- HAL、Driver 或 Device Driver 依赖 Example。
- SoC/HAL 根据 Board 名称选择实现。
- Device Driver 包含具体 Board 头文件。
- Board 驱动复制 Example 业务逻辑。
- Example 直接访问 `REG_*`、链接符号或 PinMux 寄存器。

## 6. 目标源码树

3.0 的目标结构如下。迁移期间允许旧目录短期存在，但不得继续向旧结构增加功能。

```text
bin/                     2.x/迁移期入口，不进入 3.0 安装包
tools/sdk-manifest.json  SDK 身份、版本、布局和工具链要求
tools/ecos_cli/          Python ECOS CLI 实现
tools/                   配置、构建和烧录工具
cmake/                   公共 CMake 模块、检查和固件目标封装
hal/<peripheral>/        内部 HAL 接口
drivers/<peripheral>/    稳定公共外设 Driver
devices/<device>/        外部器件 Driver
boards/<board>/          BSP 清单、资源绑定和板级实现
components/<component>/  公共软件组件
components/soc/<target>/ SoC、CPU/ISA/ABI、寄存器、能力和基础构建规则
examples/<category>/<name>/
tests/
templates/project/
templates/bsp/
docs/
```

目录名只表达职责，不要求一次提交完成全部改名。目录迁移必须和构建解析、文档及测试
一起完成，禁止只移动文件而保留原有耦合。

## 7. 清单和能力模型

### 7.1 Board 清单

Board 清单至少要描述：

```yaml
schema: 2
id: example-board
target: example-soc

build:
  default_profile: flash-xip

resources:
  console:
    driver: sys-uart
    instance: 0
  display:
    driver: st7735
    bus: qspi0
    chip_select: 0
    dc_gpio: { controller: 0, pin: 31 }
```

实际 schema 必须通过结构化解析器和 schema 校验处理，3.0 不再扩展仅靠 `awk` 匹配
缩进的 YAML 解析方式。

### 7.2 Example 清单

Example 必须声明源码和运行所需的逻辑能力：

```yaml
schema: 1
name: donut
sources:
  - main.c
  - donut.c
requires:
  - console
  - display
```

面向某个器件本身的验证可以要求 `display.controller.st7735`；普通显示应用只能要求
`display`，不得把控制器型号写死在应用代码中。

### 7.3 能力的三个层次

- Target/SoC capability：是否有 GPIO/QSPI、控制器数量、DMA 和中断能力。
- Board resource：板上是否绑定 Display/LED/Button，以及它们使用的硬件资源。
- Example requirement：应用完成其功能需要哪些逻辑资源。

三者不得合并为一组模糊的布尔开关。

## 8. 工程、CMake 和构建边界

### 8.1 工程元数据

3.0 普通工程必须使用结构化工程元数据 `.ecos/project.yml`。新工程元数据必须包含
schema、Example、Board、Target 和配置 profile，不得作为 Shell 脚本直接 `source`
执行。通过 Board 模式配置时，Target 是 Board 清单的派生结果；通过 Target 模式配置时，
Board 必须为空。设置任一模式都必须按 2.2 节的覆盖语义原子更新这两个字段。

工程可以固定 SDK 注册名称、SDK ID 和版本范围，但不得把某台主机的 SDK 绝对路径写入
可提交的工程元数据。项目固定版本无法在本机注册表中解析时必须明确失败，禁止静默回退
到全局默认 SDK。绝对路径只允许出现在用户级注册表或不可提交的本机派生状态中。

工程根目录的 `CMakeLists.txt` 只声明工程和应用组件，不得重复保存 Board、Target、
工具链或组件依赖。`.ecos/project.yml`、各类清单和 Kconfig 是输入事实来源；生成的
CMake 文件只能放在 `build/` 或 `.ecos/generated/`，不得要求用户手工编辑。

`ecos init_project` 可以保留现有命令名，但其行为必须调整为：

1. 复制唯一的 Example 应用源码和最小 CMake 工程入口。
2. 写入工程元数据。
3. 解析 Board、Target 和组件依赖。
4. 生成只属于 `build/` 或 `.ecos/generated/` 的 CMake/Kconfig 派生配置。
5. 构建时引用 SDK 中的 BSP、启动、链接和 Driver，不覆盖用户源码。

切换 Board 时只能更新工程元数据和生成文件，禁止替换 `main.c`、公共
`CMakeLists.txt` 或其他用户维护文件。

### 8.2 CMake 职责

CMake 是 SDK 3.0 唯一正式的核心构建描述系统。Ninja 是开发、CI 和发布验收的默认
生成器，因此 SDK 3.0 核心流程不得依赖 GNU Make。可以允许用户选择其他 CMake
生成器，但它们不自动进入首发支持矩阵。

构建实现必须遵循：

- Target 对应的 SoC 组件提供 CMake toolchain 文件，负责编译器、CPU/ISA/ABI 和基础
  编译链接选项，不得包含具体 Board 资源。
- 统一解析器先校验 Project、Board、Target、Component 和 Example，再生成一个供
  CMake 消费的解析结果；CLI 和 CMake 不得各自实现一套依赖选择逻辑。
- Driver、Device、BSP、Component 和 Application 映射为有明确名称和依赖的 CMake
  target，依赖通过 `target_link_libraries` 表达。
- include 路径、宏定义和编译选项使用 `target_*` 及正确的 `PRIVATE`、`PUBLIC`、
  `INTERFACE` 传播范围，禁止依赖全局目录状态拼接所有参数。
- 源文件列表来自正式 Component/Example 清单或组件声明，禁止通过递归 glob 隐式加入
  未声明源码，也禁止在多个 Board CMake 文件中重复维护应用源码列表。
- 启动文件和链接脚本由 Target/Board 清单解析后绑定到最终固件 target，不复制到普通
  工程，也不由应用 `CMakeLists.txt` 选择。
- 自定义生成步骤必须声明输入、输出和依赖；配置和构建不得修改 SDK、Board、Component
  或 Example 源码目录。
- 正式固件构建至少生成 ELF、BIN、MAP、尺寸报告和 `compile_commands.json`；HEX 是否
  生成由烧录方式决定。
- 支持通过 CMake File API 或等价的正式接口查询 target 图，不要求工具解析 Ninja
  文件、终端日志或 CMake 内部缓存格式。

CMake 最低版本为 3.20，Ninja 最低版本为 1.10，由 CLI 统一预检；安装器分别锁定
`cmake==3.31.10` 和 `ninja==1.11.1.4`，不得在不同 Board 中分别提高版本要求。默认
配置过程不使用 `FetchContent` 自动联网。

### 8.3 工具链和宿主平台策略

SDK 3.0 首个版本统一采用 xPack GNU RISC-V Embedded GCC，基线锁定为 GitHub Release
`15.2.0-1`（xpm 包版本 `15.2.0-1.1`），工具前缀为 `riscv-none-elf-`。受支持的宿主包
包括 Windows x64、GNU/Linux x64/arm64 和 macOS x64/arm64。升级工具链必须通过独立
评审和完整回归，禁止在安装时自动选择 `latest`。

工具链集成必须遵循：

- 所有宿主平台使用同一个 xPack Release。不得长期采用“GNU/Linux 使用自编译版本、
  Windows/macOS 使用 xPack”的默认组合。
- SDK 维护结构化工具链清单，记录逻辑名称、Release、宿主系统与架构、下载地址、归档
  格式、SHA-256、工具前缀和支持的 Target/ISA/ABI；安装脚本不得重复硬编码这些信息。
- ECOS CLI 根据宿主系统选择归档，负责下载、校验、解压、缓存、离线导入和安装状态
  诊断。使用 xPack 二进制不等于强制用户安装 Node.js 或 xpm。
- 工具链安装到用户级、跨 SDK 版本共享的版本化数据目录，不提交到 SDK 仓库，也不复制
  进普通工程；下载归档属于缓存，可以独立清理，已安装工具链不得仅因清理缓存而失效。
- CMake toolchain 文件使用解析后的编译器绝对路径，不依赖全局 `PATH`，也不允许各
  Board 自行声明 `CROSS` 前缀。
- 允许用户显式选择 `custom` 工具链目录，但 CLI 必须检查编译器身份、版本、目标三元组
  和所需 ISA/ABI，诊断输出必须标明当前构建不再使用 SDK 锁定的默认工具链。
- 现有 `riscv64-unknown-elf-` 自编译 GNU/Linux 工具链作为 2.x/迁移期 `legacy` 提供者
  保留，不作为 SDK 3.0 发布基线。其退出时间由迁移回归结果决定。

当前 StarrySkyL3/L3.1 使用 `rv64ifd_zifencei/lp64`，而 xPack 没有该组合的同名预编译
multilib。现有裸机构建使用 `-nostdlib` 和 SDK 自带运行库，因此可以先验证编译、汇编
和链接；如果未来改用工具链 Newlib、libstdc++ 或 `libgcc.a`，必须增加实际链接和板测，
不得为了匹配现成 multilib 未经 ABI 评审就改为 `lp64d`。

自有工具链建设不属于 SDK 3.0 首发范围。只有在需要自定义 GCC/Binutils 补丁、xPack
缺失必要 multilib、供应链策略要求或长期维护收益明确时才启动。届时必须从相同源码、
补丁、配置和 multilib 清单生成全部受支持宿主包，并建立可复现构建、许可证归档、制品
签名、SBOM 和跨平台回归；不能只恢复维护一个 GNU/Linux 二进制包。

这里的 Python 重构仅针对 ECOS 主机侧工具和编排层。RISC-V 编译、汇编、链接、调试仍
由 xPack 中的 GCC、Binutils 和 GDB 完成，Python 不重新实现编译器工具链。

### 8.4 Kconfig 配置

Kconfig 可以继续作为 3.0 的用户配置模型，但必须从 Make 构建规则中解耦：

- CLI 的 configure/menuconfig 工作流负责生成构建目录内的配置文件、C 头文件和 CMake
  可消费变量；具体命令名在阶段 A 与其余稳定 CLI 一起固化。
- CMake 配置阶段必须将 Kconfig 产物作为显式输入；配置改变后能够正确触发重新配置和
  必要的重新编译。
- Make 专用的 `fixdep` 不进入 3.0 核心构建链；头文件依赖由 CMake 和所选生成器处理。
- Board 只能提供 Kconfig 默认值和受硬件约束的选项，不能用 Kconfig 重新实现 Board
  或 Target 选择。

### 8.5 Make 兼容和迁移范围

- 从阶段 A 开始，新 Target、Board、Component 和 Example 禁止新增 SDK 自有 Make
  构建逻辑；旧 Makefile 只接受阻塞迁移的修复。
- 2.x 工程继续在 2.x 分支使用 Make。3.0 可以提供限时兼容入口，但不得同时维护两套
  相互独立的源码和依赖清单。
- SDK 自有的板卡、Kconfig 主机工具和正式 Example 属于迁移范围；3.0 核心构建和阶段 B
  垂直链路不得要求系统安装 GNU Make。
- 第三方项目可以保留自己的 Makefile，但只能通过明确的可选适配器接入，不能让核心
  SDK、普通 Example 或默认 CI 隐式恢复 Make 依赖。
- `-isolated` 若继续支持，必须从同一解析结果导出 CMake 工程，不能保留第二套 Make
  板卡和 Example 选择逻辑。

### 8.6 SDK 身份和版本唯一来源

SDK 必须通过 `tools/sdk-manifest.json` 声明自身身份。清单至少包含：

- `schema_version`、稳定 `sdk_id`、`sdk_version` 和发布通道 `channel`。
- SDK 资源布局，包括 Board、Target、Component、Example、Template 和文档入口，以及
  可使用该 SDK 的 Python CLI/schema 兼容范围。
- 所需工具链 ID、Release 和兼容范围；具体宿主资产和 SHA-256 仍由独立工具链清单提供。
- 正式发布制品的内容摘要或可关联的发布摘要；签名和 SBOM 按发布供应链策略记录。

`sdk_version` 是正式安装目录名、SDK 注册信息、项目版本约束和发布制品版本的唯一事实
来源。Python CLI 包版本、工具链版本、Git 分支名和安装目标路径都不得反向推断或覆盖
SDK 版本。发布流水线必须校验 SDK 清单版本、Git tag 和发布元数据一致；运行时安装器
不得提供允许用户把任意内容伪装成其他 SDK 版本的 `--version` 参数。

开发清单可以使用明确的预发布版本和 `development` 通道。开发注册项另外记录 Git
revision 或等价源码修订信息，但 revision 不替代 `sdk_version`，也不要求拼入正式版本
目录。正式版本清单一经发布不得在相同版本号下改变内容。

### 8.7 开发注册、正式安装和平台目录

SDK 3.0 必须区分两种部署模型：

- `development`：直接注册当前源码树，`ecos` 从该 checkout 解析资源，不复制 SDK。
  源码修改应立即对该注册项生效；源码移动或删除后由诊断命令报告失效。
- `release`：把经过发布清单约束的 SDK 制品解压或部署到稳定、版本化目录，再注册为
  正式版本。不得把整个 Git 工作区、测试产物、2.x Shell 命令或未声明开发文件直接
  `cp` 到发布目录。

默认用户级正式 SDK 根目录为：

```text
GNU/Linux  ${XDG_DATA_HOME:-~/.local/share}/ecos/sdk/<sdk_version>/
macOS      ~/Library/Application Support/ECOS/SDKs/<sdk_version>/
Windows    %LOCALAPPDATA%\ECOS\SDKs\<sdk_version>\
```

显式 `--prefix` 表示版本目录的父目录，安装器仍必须从 SDK 清单追加并校验
`<sdk_version>`，不能用目录名改变版本身份。系统级安装和管理员权限不作为默认要求。

正式安装必须先部署到同一文件系统中的临时 staging 目录，完成清单、文件范围和摘要
校验后再原子激活。目标版本已存在时：内容摘要一致则幂等成功；摘要不同则拒绝覆盖，
正式稳定版必须先显式卸载，不能通过普通更新静默替换。同一安装不得覆盖其他版本。

安装目录中的 SDK 资源视为只读。用户 BSP、用户配置、工程、工具链和下载缓存必须位于
独立用户数据目录，SDK 更新不得依赖备份再恢复安装树中的 `UserBSP`。工具链按
`<toolchain-id>/<release>/<host>` 共享安装，多个兼容 SDK 版本不得重复存放同一归档内容。

主机级 Python CLI 和全局 `ecos` 启动器必须独立于任一 SDK 版本目录。GNU/Linux 和
macOS 默认使用 `~/.local/bin/ecos`，Windows 默认使用
`%LOCALAPPDATA%\ECOS\bin\ecos.cmd` 或等价的用户级入口。`ecos sdk use` 只更新注册表
active 项，不得通过重写 PATH 来切换版本；解析器必须检查当前 CLI/schema 是否满足所选
SDK 清单声明的兼容范围。

全局 CLI 的 `bin/` 只允许包含 Python `ecos` 入口和 Windows 必需的 `.cmd` 启动器；源码
`bin/` 中的迁移期 Shell 脚本不得进入正式安装或全局 CLI 目录。SDK 版本目录不得用自身
`bin/` 抢占全局 SDK 选择。开发注册和正式安装都必须配置同一套 Bash、Zsh、Fish、
PowerShell 补全生成能力。

### 8.8 SDK 注册表、路径解析和多版本

主机必须维护用户级 SDK 注册表。注册项至少包含注册名称、SDK ID、版本、规范化根路径、
`checkout` 或 `release` 类型，以及可用时的 revision 或发布摘要。注册表必须采用带
schema 版本的结构化格式、原子写入和并发更新保护，位置遵循宿主平台用户配置目录：

```text
GNU/Linux  ${XDG_CONFIG_HOME:-~/.config}/ecos/sdks.json
macOS      ~/Library/Application Support/ECOS/sdks.json
Windows    %APPDATA%\ECOS\sdks.json
```

CLI 至少提供等价于以下能力的稳定接口；最终命令拼写在阶段 A 固化：

```text
ecos sdk register <path> [--name <name>]
ecos sdk list
ecos sdk current
ecos sdk use <name-or-version>
ecos sdk pin <name-or-version>
ecos sdk unregister <name>
ecos sdk doctor
```

注册只记录和校验路径，不复制源码。注册时必须读取 SDK 清单、规范化路径、校验必要资源
布局，并处理重复名称、同版本不同摘要、路径移动和不可访问目录。`unregister` 默认只删除
注册关系，不删除 checkout；删除正式安装内容必须是范围明确的独立操作。

每条 CLI 工作流必须通过同一个路径解析器获得 SDK 上下文，禁止各命令分别使用
`Path(__file__).parents[...]`、固定 `~/.local/ecos-sdk` 或当前工作目录猜测资源。解析优先级
固定为：

1. 当前命令显式传入的 `--sdk` 注册名、版本或路径。
2. 当前工程元数据固定的 SDK 注册名、ID 和版本约束。
3. `ECOS_SDK_HOME` 兼容变量；该入口必须经过相同清单和布局校验。
4. 用户注册表中的 active SDK。
5. 仅在从 SDK checkout 源码入口运行时，根据入口位置识别当前 checkout。

解析成功必须生成可注入、可测试的 `SdkContext`，统一提供 SDK 根目录、版本、类型及
Board、Target、Component、Example、Template、工具链要求等资源路径。路径包含空格、
非 ASCII 字符、Windows 盘符或符号链接时必须保持相同语义。高优先级选择存在但无效时
必须报告选择来源和修复建议，不得悄悄尝试低优先级 SDK。

多版本管理由注册表负责，项目固定版本优先于全局 active 版本，`--sdk` 只覆盖单次命令。
不同 SDK 可以引用相同工具链 Release；工具链解析由 SDK 清单要求和工具链安装状态共同
决定，不把当前 Shell 的 `PATH` 当作版本选择事实来源。

## 9. AI 与自动化接口边界

### 9.1 CLI 的定位

CLI 是人、AI 和 CI 共同使用的稳定编排接口，应保持完整但克制。CLI 只负责参数处理、
调用公共解析/构建能力和呈现结果，不得在每个子命令中重复实现 Board、Target、依赖
或能力解析逻辑。交互界面和机器接口必须产生相同的解析结果。

命令层级、命名、实现状态、参数和兼容性由 [ECOS 3.0 命令行接口](cli.md) 统一管理。
只有其中标记为“已实现”的 Python 命令才能进入正式帮助和自动补全。

SDK 3.0 的正式 CLI 必须重构为可安装的 Python 包，并通过 `pyproject.toml` 声明
`ecos` 命令入口。最低 Python 版本、依赖锁定方式和分发形式在阶段 A 固化；Windows、
GNU/Linux 和 macOS 必须运行相同的 Python 模块和命令语义，禁止按宿主系统复制三套
业务实现。

Python 主机工具必须遵循：

- 资源解析、schema 校验、工程修改、工具链管理和构建编排实现为可测试的 Python
  模块；命令处理层只做参数映射和输出呈现。
- 文件和路径操作使用跨平台 API，内部路径使用 `pathlib.Path` 等结构化对象，不拼接
  `/`、假定盘符或依赖符号链接一定可用。
- 调用 CMake、Ninja、编译器、烧录器等外部程序时传递参数数组并检查退出状态，禁止将
  工程或清单内容拼成 Shell 命令，也不得默认使用 `shell=True`。
- 下载、SHA-256 校验、ZIP/TAR 解压、复制、临时目录和缓存管理由 Python 实现，不把
  `wget`、`curl`、`tar`、`unzip`、`cp`、`sed`、`find`、`realpath` 等命令作为核心依赖。
- `ecos` 的正常配置和构建不依赖用户预先修改工具链 `PATH` 或执行
  `eval "$(ecos env)"`。SDK 安装阶段必须识别 Bash、Zsh、Fish、PowerShell，在对应的
  用户启动文件中以明确的 ECOS 标记块幂等配置全局 CLI 的 PATH 和自动补全；不得修改
  标记块之外的用户内容。启动文件不得持久写死某个 SDK 的 `ECOS_SDK_HOME`，否则会绕过
  active SDK 并把非项目命令固定在单一版本；项目 pin 仍按更高优先级生效。该变量只作为
  用户显式设置的单次兼容覆盖。必须提供 `--shell` 覆盖、`--shell-profile` 指定路径和
  `--shell none` 禁用入口。CMD 不提供可编程自动补全，Windows 默认使用 PowerShell。
- 主机配置流程不得执行仓库中预先生成的 Linux ELF。Kconfig 解析和非交互配置优先复用
  成熟的 Python Kconfig 实现；`menuconfig` 必须有跨平台实现或明确的受支持终端边界。
- 串口监视、端口枚举和超时处理使用跨平台 Python 串口能力，不以 `minicom` 作为正式
  依赖；烧录器仍可作为外部程序，但发现、参数构造和诊断由 CLI 统一完成。
- Python 依赖必须锁定并进入许可证与供应链审查。CLI 的核心解析和查询测试不得访问
  网络，工具链下载等联网行为必须由显式命令触发。

SDK 安装、注册、版本目录和路径解析必须遵守 8.6 至 8.8。`tools/install.py` 负责开发
checkout 引导注册和正式发布制品安装；`bin/ecos*` 与 `tools/scripts/*.sh` 只作为源码树
中的 2.x/迁移期入口保留。帮助和自动补全只能公开已完成 Python 迁移的命令。仓库中现有
Linux 版 Kconfig/fixdep 构建产物不得进入 3.0 发布包。

3.0 CLI 至少要覆盖以下工作流；具体命令名在阶段 A 固化，旧命令可以作为带弃用提示
的兼容别名：

- 发现：列出 Board、Target、Example 和 Component。
- 检查：查看资源的清单、能力、依赖、schema 和兼容关系。
- 工程：创建工程、查看当前解析状态和切换 Board/profile。
- 验证：在不编译的情况下检查 schema、依赖、能力、工具链和工程状态。
- 执行：配置、构建、清理、烧录和监视。
- 诊断：检查 SDK、工具链、烧录器、串口和环境，并给出修复建议。

不为同一语义增加多个近似命令。发现和检查应优先采用一致的资源模型，例如
`list <kind>` 和 `describe <kind> <id>`，而不是为每种资源设计一套独立参数规则。

### 9.2 机器可读契约

所有发现、检查、验证和诊断命令必须支持 `--format json`。构建、烧录等长任务至少
要提供结构化最终结果；后续可以增加 JSON Lines 事件流，但不作为 3.0 首发条件。

机器模式必须满足：

- 输出包含 CLI/schema 版本、命令状态、数据和诊断数组。
- JSON 数据只写入标准输出，日志和进度写入标准错误，不混入颜色控制字符。
- 使用文档化的稳定退出码区分成功、用法错误、配置错误、能力不匹配、构建失败和
  外部工具失败。
- 诊断至少包含稳定错误码、严重级别、消息、相关资源或字段路径、实际值，以及可行时
  的期望值和修复建议。
- 清单、帮助、schema 和命令参数由同一事实来源生成或接受一致性测试，避免文档与实现
  分别维护后漂移。

面向人的默认文本可以优化可读性，但不得成为自动化工具必须解析的唯一输出。

### 9.3 非交互、确定性和安全性

- 参数完整时禁止出现交互式提问；所有交互命令必须提供等价的非交互参数。
- 命令必须支持显式指定工程目录，不能要求 AI 依赖当前工作目录猜测目标工程。
- 相同 SDK 版本、工程元数据和工具链输入必须得到相同的依赖解析和构建计划。
- 校验和查询命令必须无副作用；修改工程状态的命令应支持 `--dry-run`。
- 重复执行设置 Board、生成派生配置等操作必须幂等。
- 默认禁止覆盖用户维护的源码。确需覆盖或删除时必须显式传入 `--force`，并在执行前
  列出准确目标；`--dry-run` 下不得写入文件。
- 默认流程不得执行清单中的任意 Shell 代码，也不得因解析工程而自动访问网络。

### 9.4 本地知识和可验证示例

- CLI 的 `help`、资源清单、schema、公共 API 文档和迁移文档必须随 SDK 本地提供，
  离线环境也能完成查询。
- 公共 API 文档中的最小示例必须进入编译测试，避免 AI 复制已经失效的片段。
- 每个 schema 必须提供最小合法样例、字段说明和至少一个典型错误样例。
- 可以提供面向编码助手的索引或上下文文件，但它只能汇总正式文档，不得成为新的配置
  来源，也不得要求用户绑定特定模型、编辑器或云服务。

### 9.5 VS Code 扩展边界

SDK 3.0 计划提供官方 VS Code 扩展。扩展是 Python `ecos` CLI 的 TypeScript 薄客户端，
负责编辑器集成和交互呈现，不直接解析 Board/BSP 内部文件、不自行解析依赖，也不复制
工具链下载、CMake 配置或烧录逻辑。扩展与 CLI 通过文档化的 JSON schema 和退出码通信，
并声明兼容的 CLI/schema 版本范围；版本不兼容时必须停止执行并给出升级方向。

首个稳定扩展至少提供：

- 识别当前 ECOS 工程，展示 SDK、Board、Target、profile、工具链和最近构建状态。
- 通过统一资源选择界面发现和切换 Board/profile，并在执行前展示兼容性检查结果。
- 提供配置、构建、清理、烧录和串口监视命令；使用 VS Code Task Provider 暴露可重复
  执行的任务，并使用进度、取消和 Output Channel 呈现长任务。
- 将 CLI 结构化诊断映射到 VS Code Problems；编译导航使用 CMake 生成的
  `compile_commands.json`，不得另行维护 include 路径、宏定义和编译参数。
- 在 Target/BSP 声明调试能力和调试服务器配置时生成调试配置，调用 xPack GDB 和已有
  VS Code 调试适配器。3.0 不自行实现 Debug Adapter，也不假定所有板卡都支持调试。
- 提供工具链状态与安装入口，但下载必须由用户显式触发，实际安装仍由 Python CLI
  完成；扩展激活或打开工程时不得自动下载工具、构建、烧录或修改工程。

安全和兼容要求：

- 扩展必须声明并实现 Workspace Trust。未信任工作区只允许无副作用的工程识别、资源
  描述和 schema 查询；配置、构建、下载、烧录、调试及串口操作必须禁用。
- 调用 CLI 和任务时使用进程参数数组，不拼接 Shell 命令；包含空格、非 ASCII 字符和
  Windows 盘符的工作区路径必须进入测试。
- 多根工作区中的状态、任务和输出必须绑定到具体工程目录，禁止依赖 VS Code 进程的
  当前工作目录猜测项目。
- 桌面扩展在 Windows、GNU/Linux 和 macOS 上使用同一代码库。Remote SSH、WSL 和
  Dev Containers 中应在工作区所在的扩展宿主执行 CLI 和选择对应宿主工具链；完整远程
  支持是否进入首发矩阵由阶段 C 的验证结果决定。
- 扩展不得静默修改用户级 VS Code 设置，也不强制依赖 CMake Tools 或某个 C/C++ 扩展；
  可选集成必须进行能力检测并在缺失时保留基础 CLI 工作流。

扩展测试至少包括 CLI JSON fixture 契约测试、命令与状态单元测试、受信任/未信任工作区
测试，以及 Windows、GNU/Linux 和 macOS 上的扩展宿主集成测试。涉及烧录和调试的功能
还必须使用模拟外部进程，并为正式支持的硬件组合保留板测记录。

## 10. 公共 API 和兼容策略

### 10.1 公共 API

以下内容承诺在 3.x 内遵循兼容和弃用流程：

- `drivers/*/include/` 中明确标记为 public 的头文件。
- `devices/*/include/` 中的器件公共 API。
- BSP 公共 API 和能力查询接口。
- 工程、Board、Target、Component 和 Example 的正式 schema。
- SDK 身份清单、用户注册表、项目 SDK 固定信息及其路径解析优先级。
- `ecos` CLI 中在用户文档声明为稳定的命令和参数。
- 面向应用工程公开并记录为稳定的 CMake 函数、变量和 target 名称。

SoC 寄存器、HAL、LL、生成文件、私有 CMake 模块和构建内部变量不属于稳定 API。

### 10.2 2.x 兼容

- `2.1-dev`/后续 2.x 分支以修复和稳定为主，不接受 3.0 结构性回灌。
- 3.0 允许修改目录、BSP schema、工程元数据和旧 HAL API。
- 3.0 必须提供 2.x BSP 和应用迁移文档，但不要求无修改编译所有 2.x 工程。
- 兼容 Wrapper 必须有明确移除版本，禁止永久并行维护两套 Driver 实现。
- 旧 `TEMPLATE=` 工程元数据可以由迁移工具读取，新工程不得继续写入旧格式。

## 11. 分阶段交付

### 阶段 A：契约和构建骨架

- 固化 Target、Board、Component、Example schema。
- 固化 `tools/sdk-manifest.json` schema 和 SDK 版本唯一来源，建立 Git tag、Python CLI
  兼容版本及发布元数据的一致性检查。
- 实现 SDK 注册表、统一 `SdkContext` 和固定解析优先级，提供 register/list/current/use/
  pin/unregister/doctor 的机器可读最小链路，并覆盖多版本和失效路径诊断。
- 将安装器拆分为不复制源码的 development 注册模式和部署清单文件的 release 模式；
  固化三平台默认版本目录、staging 校验、原子激活、同版本冲突和卸载边界。
- 建立结构化解析和校验。
- 建立 `pyproject.toml`、Python 包目录和 `ecos` 命令入口，确定最低 Python 版本、依赖
  锁定、测试和三平台安装方式；完成资源发现、schema 校验和机器可读诊断的最小链路。
- 确定受支持的 CMake/Ninja 最低版本，建立公共 CMake 模块和 Target toolchain 文件。
- 建立工具链清单和提供者接口，锁定 xPack `15.2.0-1`，实现宿主识别、下载、SHA-256
  校验、缓存、离线导入、`custom` 覆盖和机器可读诊断。
- 建立统一工程配置、构建入口和生成目录，完成 Kconfig 到 CMake 的配置链路。
- 保证 Board 切换不修改用户源码。
- 固化 CLI 资源模型、JSON 输出、退出码和诊断格式，并建立契约测试。
- 阶段 A 完成后冻结旧 Make 构建功能，只允许迁移修复。

### 阶段 B：最小垂直链路

至少完成以下三条链路，并同时覆盖 StarrySkyL3_1 和 StarrySkyL4：

- `hello`（目录 `example/get_start/hello/`）：验证 Console。
- `peripherals/gpio-blink`：验证逻辑 LED/GPIO 资源。
- `display/donut`：验证 BSP、Device Driver、总线 Driver 和显示资源。

三条链路必须使用同一套 CMake 模块和 Ninja 构建，并在核心流程中不调用 GNU Make。
工程创建、配置、工具链解析和构建编排必须经过 Python `ecos` CLI，不能回退到 Shell
脚本实现。
这一阶段完成前，不批量迁移其余 Example。

### 阶段 C：扩大驱动和板卡覆盖

- 迁移其他公共 Driver 和 Device Driver。
- 按实际硬件验证结果补充 Board 能力。
- 在 Windows x64、GNU/Linux x64/arm64 和 macOS x64/arm64 上验证工具链安装与最小
  CMake 构建；固件回归必须覆盖 SDK 声明的全部 ISA/ABI 组合。
- 将 Board/BSP 管理、工程迁移、Kconfig、烧录、串口监视和补全生成迁移到 Python CLI，
  删除这些流程对 Bash、GNU Coreutils、`minicom` 和预编译 Linux 主机工具的依赖。
- 建立 VS Code 扩展工程并发布预览版，完成工程识别、资源状态、Board/profile 选择、
  工具链诊断以及配置/构建/清理 Task Provider；用 CLI 契约 fixture 建立扩展测试。
- 将现有 Example 分类迁移到 `examples/`。
- 将具有明确断言的程序迁移到 `tests/`。
- 将 SDK 自有 Board、Component 和主机构建工具迁移到 CMake，删除对 `build_conf.mk`、
  `config.mk` 和 Make 专用 `fixdep` 的新架构依赖。

### 阶段 D：清理和发布

- 删除重复的板卡专用 Example 源码。
- 删除根目录旧 `example/` 和其中的构建产物。
- 移除已到期的兼容 Wrapper。
- 移除 SDK 3.0 核心路径和正式 Example 中的 Make 构建入口。
- 移除 SDK 3.0 发布包中的旧 Shell 业务实现和预编译 Linux Kconfig/fixdep 工具；仅可
  保留不含业务逻辑的可选启动 Wrapper。
- 生成由 `tools/sdk-manifest.json` 约束的版本化发布制品，在三平台验证默认安装目录、
  自定义 prefix、并存版本、项目固定版本、升级冲突、卸载和 checkout 注册流程。
- 发布与 SDK 3.0 CLI/schema 版本兼容的稳定 VS Code 扩展，完成烧录、串口监视、诊断
  映射和具备调试能力 Target 的调试配置，并建立三平台扩展测试矩阵。
- 完成 SDK 3.0、BSP 和应用迁移文档。
- 建立受支持宿主、Board/Example 和 ISA/ABI 的 CI 构建矩阵。

## 12. 3.0 发布验收条件

SDK 3.0 发布前必须满足：

- `tools/sdk-manifest.json` 是 SDK 版本和发布布局的唯一事实来源；Git tag、发布制品和
  注册信息与其一致，不使用 Python CLI 或工具链版本推断 SDK 版本。
- development 模式注册 checkout 且不复制 SDK；release 模式只部署清单声明文件到
  平台默认或显式 prefix 下的版本目录，并通过 staging、摘要校验和原子激活完成安装。
- 至少两个 SDK 版本可以并存；全局 active、项目 pin 和单次 `--sdk` 按规定优先级解析，
  缺失的高优先级 SDK 明确失败且不会回退到其他版本。
- SDK 注册表在 Windows、GNU/Linux 和 macOS 上完成 register/list/use/unregister/doctor
  回归，路径移动、重复名称、同版本摘要冲突、空格和非 ASCII 路径具有测试覆盖。
- 正式安装目录不保存用户 BSP、用户配置、下载缓存或重复工具链；兼容 SDK 版本能够
  共享同一 xPack Release，清理下载缓存不会破坏已安装工具链。
- 主机级全局 CLI 独立于 SDK 版本目录，其 `bin/` 只包含生成的 Python 入口和 Windows
  `.cmd` 启动器；升级能够清理旧 Shell 命令，帮助与补全不暴露尚未完成 Python 迁移的
  命令。切换 active SDK 不重写 PATH，启动文件不持久绑定 `ECOS_SDK_HOME`。
- 已迁移的每个 Example 只有一份业务源码，不包含 Board 名称条件分支。
- 同一工程可以通过修改 Board 元数据为至少两个兼容板卡构建，不修改应用源码。
- Example 不直接访问寄存器、PinMux 或具体板卡 GPIO 编号。
- Board/Target/Component/Example 清单全部经过 schema 校验。
- 不支持的能力组合在编译前失败，并指出缺少的资源。
- 默认工具链在 Windows、GNU/Linux 和 macOS 的声明宿主架构上可以从工具链清单完成
  安装和校验，解析结果均为 xPack `15.2.0-1`；安装过程不依赖系统已有 xpm。
- 从安装、工程创建、Board 选择、配置、构建、烧录到串口监视的正式 CLI 流程由同一套
  Python 实现提供；Windows 验收环境不安装 Bash、GNU Coreutils 或 `minicom`。
- Python 包可在所有声明宿主环境安装，依赖版本可复现且许可证清单完整；路径含空格和
  非 ASCII 字符时，至少完成工程创建、配置和构建回归。
- VS Code 扩展不解析内部 Board/BSP 构建事实，仅通过公开 CLI/schema 完成工程识别、
  Board/profile 选择、工具链检查、配置和构建；扩展与不兼容 CLI 配对时能够明确拒绝。
- 未信任工作区只能调用经过契约标记的只读 CLI 查询，不会触发工程写入、下载、构建、
  烧录、调试或串口访问；Windows、GNU/Linux 和 macOS 上至少完成受信任工作区的工程
  识别和构建集成测试。
- C1/C2、T1、L3/L3.1 和 L4 使用锁定工具链完成编译链接回归，并校验 ELF 架构、ABI、
  入口、段布局和固件尺寸阈值；L3/L3.1 不得因工具链迁移静默改用 `lp64d`。
- 构建产物全部位于工程构建目录，不污染 SDK、Example、BSP 或 Device 源码。
- SDK 自有正式 Example 通过 CMake 配置并使用 Ninja 构建；在未安装 GNU Make 的干净
  环境中，阶段 B 垂直链路能够完成配置和构建。
- 切换 Board 后重新配置 CMake 即可完成构建，不修改应用 `CMakeLists.txt` 或源码。
- CMake target 的源码、include、宏定义和依赖关系可通过正式接口查询，并生成有效的
  `compile_commands.json`。
- StarrySkyL3_1 和 StarrySkyL4 至少完成 hello、GPIO 和 display 垂直链路构建。
- 每个受支持组合均有可重复的 CI 构建记录；涉及硬件语义的能力有对应板测记录。
- 2.x 到 3.0 的 BSP 和应用迁移文档已经发布。
- 自动化工具可以通过正式 CLI 的机器模式发现 Board/Example、检查兼容关系、创建工程
  和执行构建，全程无需解析面向人的表格或日志。
- 发现、检查、验证和诊断命令的 JSON 输出、退出码及典型失败诊断具有契约测试。
- 工程修改命令通过测试证明 `--dry-run` 无写入、重复执行结果幂等，且不会覆盖用户源码。

## 13. PR 评审边界

3.0 PR 在合并前必须回答以下问题：

1. 这项变化属于 Target/SoC、HAL、Driver、Device、BSP、Component 还是 Example？
2. 是否产生了从底层到上层或从公共组件到具体 Board 的反向依赖？
3. 是否把板卡引脚、寄存器或构建参数引入了 Example？
4. 是否新增了另一份板卡专用业务源码？
5. 是否声明并校验了所需能力，而不是依赖构建失败碰运气？
6. 新增抽象是否至少解决两个实现之间的真实差异？
7. 是否提供了与影响范围相称的构建、仿真或硬件验证？
8. 新增能力是否能通过公开 CLI/schema 发现和验证，而不要求解析源码或人类日志？
9. 新增诊断是否包含稳定错误码、问题位置和可操作的修复建议？
10. 是否新增或扩展了 Make 构建逻辑，或者在 CMake 和清单之间制造了第二份事实来源？
11. CMake 依赖和编译属性是否通过 target 正确传播，而不是依赖目录级全局状态？
12. 工具链版本、宿主资产、校验值和 Target 兼容信息是否来自统一工具链清单？
13. 是否向 3.0 核心流程新增了 Shell、Unix 命令、宿主专用路径或预编译 Linux 主机
    工具依赖，而没有对应的跨平台 Python 实现和测试？
14. VS Code 扩展是否只消费公开 CLI/schema，并正确处理版本兼容、工作区信任、取消和
    多根工作区，而没有复制构建或资源解析逻辑？
15. SDK 版本、安装目录和项目固定信息是否都来自 `tools/sdk-manifest.json` 与注册表，
    而不是从 CLI 版本、Git 分支、路径名或当前工作目录推断？
16. 新命令是否通过统一 `SdkContext` 解析资源，并遵守 `--sdk`、项目 pin、兼容环境变量、
    active SDK 和 checkout 识别的固定优先级？
17. 安装变化是否分别覆盖 development 注册与 release 部署，且没有复制 Git 工作区、
    覆盖其他 SDK 版本、把用户数据写入 SDK 或重复安装共享工具链？

不满足这些边界的改动应先调整设计，而不是通过增加 Board 条件宏或复制目录绕过。

## 14. 延后决策

以下决策只有在阶段 B 完成并取得度量后才进入评审：

- 是否把 Unix Makefiles 等其他 CMake 生成器加入正式支持矩阵。
- 是否引入独立组件管理器和远程组件仓库。
- 是否增加通用异步、DMA 和 RTOS Driver 模型。
- 是否支持运行时 Board 探测或动态 Driver 注册。
- 是否把 isolated 工程升级为正式的离线 SDK 导出格式。
- 是否启动 ECOS 自有跨平台 RISC-V 工具链建设；启动前必须先确认 xPack 的实际缺口和
  自维护成本，并形成独立的构建、发布与安全维护计划。

任何延后事项都不能阻塞 Target/BSP/Driver 分层、单一 Example 源码、统一 CMake
构建和机器可读自动化接口这些核心目标。
