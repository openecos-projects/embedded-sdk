# ECOS Embedded SDK 3.0.0

ECOS Embedded SDK 3.0 是一次围绕工程模型、构建工具和公共接口的主要版本升级。本版本面向 StarrySky 及兼容 RISC-V 平台，采用轻量裸机和静态链接的默认运行方式，通过统一的 Python CLI、CMake/Ninja 和板卡能力描述，提供从工程创建到构建、烧录和串口监视的开发流程。

## 主要变化

### 统一工程与构建流程

- 使用 Python `ecos` CLI 统一管理 SDK、工具链、工程配置和构建。
- 核心构建使用 CMake/Ninja，不再依赖旧版 Shell 工程入口或 GNU Make。
- 支持从 Example 创建位于 SDK 目录之外的独立工程，避免直接修改 SDK 示例源码。
- 提供 `validate`、`configure`、`menuconfig`、`build`、`clean` 和 `doctor` 等工作流。
- 构建输出包含 ELF、BIN 和 `artifacts.json`，记录固件身份、ISA/ABI、大小、摘要以及源码和配置指纹，便于追踪和自动化验证。

### Target、Board、Driver 与 BSP 分层

- 分离 SoC/Target 和具体 Board 的职责，板卡连接与默认配置由清单描述。
- 通过公共 Driver、BSP 和板级资源接口使用外设，减少应用对 SoC 寄存器和固定引脚的依赖。
- 在构建前检查 Example 能力需求、组件依赖、Board 资源以及目标架构兼容性。
- 支持 Board、Target 和构建 profile 的查询与工程选择；可用组合以当前清单为准。

### 跨平台工具链与 SDK 管理

- 锁定 xPack GNU RISC-V Embedded GCC `15.2.0-1`，按宿主选择归档并进行摘要校验。
- 安装器管理私有 Python 依赖、CMake 和 Ninja，减少对系统全局环境的依赖。
- 支持 SDK 注册、切换、工程 pin、健康检查和多版本管理。
- 支持 Bash、Zsh、Fish 和 PowerShell 补全及启动配置。
- 对同名 SDK 注册冲突提供替换注册、重新部署或使用其他注册名的解决提示。

### 公共组件与示例

- 提供统一的 Core Runtime 错误语义、日志和 panic 接口，以及裸机 libc 子集。
- 提供 GPIO、UART、Timer、I2C、QSPI 等公共接口；特定 Target 还提供中断、CLINT、SDRAM、RCU、PWM、RTC 等能力。
- 集成 SFUD、FatFs、LightCoroutine、Letter-Shell 等组件，并提供文件编辑器、FCC 和协作式 mini-os 示例。
- T1-Pico 提供 TinyStories 量化模型故事续写示例；真实运行需要自行准备模型及词表。
- 示例附带用途、接线或准备条件、运行步骤及预期现象。组件可用性以所选 Board/Target 的能力为准，不表示所有功能在所有板卡上均可用。

### 烧录、串口与自动化

- 通过 Board 声明的 mass-storage provider 烧录工程固件。
- `ecos flash --device DIRECTORY` 可指定下载器挂载目录；该参数不是 USB 序列号或串口设备路径。
- `ecos monitor --port PORT` 支持指定串口，并提供输出匹配等自动化监视能力。
- CLI 提供 JSON 输出及离线 schema 查询/校验，便于 CI、编辑器和其他工具使用。
- 提供 VS Code 扩展预览集成；扩展单独分发，不包含在 SDK 归档中。

## 环境要求

| 项目 | 版本或要求 |
| --- | --- |
| Python | 3.9 或更高 |
| 交叉工具链 | xPack GNU RISC-V Embedded GCC 15.2.0-1 |
| 受管 CMake | 3.31.10 |
| 受管 Ninja | 1.11.1.4 |
| 宿主构建验证目标 | GNU/Linux x86_64、GNU/Linux arm64、Windows x64、macOS Intel / Apple Silicon |

安装器会按需下载上述工具及 Python 依赖。SDK 归档不捆绑全部宿主工具链，因此不属于完整离线安装包。`--archive` 用于离线导入交叉工具链；Python、CMake 和 Ninja 仍需要可用的包源、本地缓存或另行准备的离线依赖。

## 安装与快速开始

从本 Release 下载并校验 SDK 的 `.tar.gz` 或 `.zip` 归档，解压后在归档根目录运行：

```bash
python3 tools/install.py
```

Windows 可使用 `python tools/install.py`。按安装器输出重新加载 Shell 配置或打开新终端。正式 release 模式会安装到版本目录并注册 SDK；GNU/Linux 默认 SDK 路径为：

```text
~/.local/share/ecos/sdk/3.0.0
```

创建并构建独立工程：

```bash
ecos sdk list
ecos project create hello --name my-hello --path /path/to/workspace \
  --board starrysky-l4-c1
cd /path/to/workspace/my-hello
ecos validate
ecos build
```

连接匹配的下载器与串口后：

```bash
ecos flash
ecos monitor --port <串口设备>
```

如需明确固定 SDK，可在工程目录执行：

```bash
ecos sdk pin 3.0.0 --project .
```

详细参数请参考 SDK 中的 `docs/install.md`、`docs/cli.md`、`docs/api/` 和命令帮助。

## 实机验证范围与结果

本次发布前的实机验证受测试时间、硬件和外设条件限制，仅覆盖以下三类实物板卡：StarrySky L4C1（L4）、StarrySky T1-Pico 和 StarrySky C2-Pico。C2-Pico 在 SDK 清单中的 Board ID 为 `starrysky-c2`。测试使用各板卡的 CI 默认 profile，结果不能推导到其他 profile、其他板卡或未接入的外设。

当前 CI 固件矩阵共包含 70 个默认 profile BIN：L4C1 27 个、T1-Pico 22 个、C2 21 个。CI 构建成功仅表示固件可以构建，不等同于实机通过。本次仅将测试记录中有明确通过证据的项目列为实机验证通过，共 31 项：

| 板卡 | Board ID / CI 默认 profile | 已确认通过的项目 | 数量 |
| --- | --- | --- | ---: |
| StarrySky L4C1 | `starrysky-l4-c1` / `memory` | `archinfo-chip-id`、`blink`、`button-state`、`coroutine-smoke`、`crc-compute`、`display-donut`、`gpio-basic`、`gpio-tm1650`、`hello`、`i2c-aht20`、`i2c-at24c64`、`i2c-scan`、`letter-shell-smoke`、`rcu-status`、`spi-master-st7735`、`uart-esp01s` | 16 |
| StarrySky T1-Pico | `starrysky-t1-pico` / `flash_xip` | `button-state`、`clint-clock`、`coroutine-smoke`、`gpio-basic`、`gpio-interrupt`、`hello`、`i2c-scan`、`letter-shell-smoke`、`mini-os`、`plic`、`rcu-pll`、`sdram-init`、`timer-interrupt` | 13 |
| StarrySky C2-Pico | `starrysky-c2` / `flash-xip` | `archinfo-chip-id`、`coroutine-smoke` | 2 |

除上表外的项目本次没有形成实机 PASS 结论。其中一部分因测试时间、外设、接线或测试条件未进行实际测试；内部记录中已有异常、阻塞或证据不足的项目也不计入通过数，后续需要补充条件并复测。未进行实际测试不等同于功能失败，也不构成对相应功能的可用性承诺。

本次未覆盖：

- SDK 中除上述三类板卡以外的其他板卡；
- C2 的 `psram-data` profile；
- 上述板卡中未列入通过表的其他例程、外设组合和启动场景；
- T1-Pico `llm-chat` 的真实模型推理。

T1-Pico 的 `llm-chat` CI BIN 只包含 1 字节占位模型和词表，状态为 `BUILD-ONLY`，仅用于验证编译和链接，不能用于有效推理验收。真实模型运行需要单独准备模型和 tokenizer，重新构建并进行独立实机测试。

L4C1 的 `memory` 固件应按对应下载器或加载器流程验证；不能仅凭复制成功推断 Flash 掉电自启动。Flash 启动固件的掉电重启能力也不因构建成功而自动成立。

## 自动化验证与固件矩阵

CI 配置包含多宿主安装和最小构建、Python 3.9 兼容性检查、CLI 测试，以及 VS Code 扩展测试和打包检查；最终结果以本版本对应的 GitHub Actions 工作流为准。

Linux x64 的例程固件矩阵限定为 L4C1、T1-Pico、C2 各自兼容的例程。CI artifact 中包含同前缀 README、`manifest.json` 和覆盖固件与说明文档的 `SHA256SUMS`。固件矩阵 artifact 与 SDK Release 归档是不同的交付物。

## 兼容性与已知限制

3.0 是主要版本升级，旧版工程和命令流程不保证直接兼容：

- 建议从 3.0 Example 创建新工程，再迁移应用源码和依赖，不直接沿用旧模板的板卡 Makefile。
- 使用 `ecos project create` 创建工程；日常配置和构建使用工程目录中的 `ecos configure` / `ecos build`。
- 不再依赖旧版 Shell 工程入口和持久设置的 `ECOS_SDK_HOME`；新工程建议通过 SDK 注册和 pin 固定依赖。
- 应用优先使用 `ecos/driver/*`、`ecos/bsp/*` 和公共组件接口，避免直接依赖内部 HAL/LL。
- `ecos sdk unregister` 只移除注册关系，不删除 SDK 文件。
- 当前完整例程矩阵仅构建默认 profile；C2 的 `psram-data` 不在这套矩阵覆盖内。
- `flash-fatfs` 只挂载已有文件系统，不自动格式化，不执行完整文件写读验收。
- `flash-sfud` 主要验证初始化和器件识别，不代表擦除、编程及全容量数据保持已通过。
- `letter-shell-smoke` 只执行一次 help 解析，不是持续交互式 Shell；交互可参考相应应用示例。
- GPIO 中断示例没有软件消抖；SDRAM 示例只验证一次 4 KiB 读写，不是全容量压力测试。
- 不提供通用 J-Link/OpenOCD/串口下载 provider 的兼容承诺；实际烧录能力以 Board 配置和 CLI 实现为准。
- VS Code 扩展为预览集成，其版本和分发独立于 SDK。

## 发布文件与完整性验证

正式发布计划提供：

- `ecos-embedded-sdk-3.0.0.tar.gz`
- `ecos-embedded-sdk-3.0.0.zip`
- SPDX SBOM（准确文件名以 Release 附件为准）
- `SHA256SUMS`

下载附件后可执行：

```bash
sha256sum -c SHA256SUMS
```

请将校验清单及其引用的附件放在同一目录；只下载部分附件时，缺少文件的项不能算校验通过。如发布工作流成功生成来源证明，也可通过 GitHub CLI 验证：

```bash
gh attestation verify ecos-embedded-sdk-3.0.0.tar.gz \
  -R openecos-projects/embedded-sdk
```

## 问题反馈

反馈问题时请附带 SDK/CLI 版本、宿主系统、Board 与硬件版本、profile、复现步骤、完整日志，以及固件 SHA-256 或 `artifacts.json`，以便定位构建与硬件差异。
