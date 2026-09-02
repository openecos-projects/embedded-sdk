# ECOS 3.0 命令行接口

本文管理 ECOS SDK 3.0 的 CLI 命令层级、命名规则、实现状态和公共行为。新增、重命名或
废弃命令时，必须同时更新本文、Python 参数解析、帮助、四类 Shell 补全和对应测试。

本文中的状态含义如下：

| 状态 | 含义 |
| --- | --- |
| 已实现 | 已由 Python CLI 注册，可以出现在帮助、补全和用户文档中。 |
| 计划中 | 已确定命令职责，但尚未实现；不得出现在正式帮助和补全中。 |
| 已废弃 | 仅为迁移保留，执行时必须给出替代命令和移除版本。 |

当前 CLI 包版本为 `3.0.0.dev0`。SDK 版本以 `tools/sdk-manifest.json` 为唯一来源，不能
根据 CLI 包版本推断 SDK 版本。

## 1. 命令模型

ECOS 采用“管理能力分组，高频当前工程操作扁平化”的混合模型。

分组命令由三个基本部分组成：

```text
ecos project create
|    |       |
|    |       `-- 动作：创建工程
|    `---------- 资源组：工程
`--------------- Python CLI 入口
```

从命令解析角度看，`ecos` 是可执行程序，`project` 是一级命令组，`create` 是二级子命令。
命令的正确拼写是 `create`，不得使用 `creat`。

以下管理操作必须分组，因为脱离资源名称后容易产生歧义：

```bash
ecos project create hello
ecos sdk list
ecos toolchain status
ecos board list
```

以下操作默认作用于当前工程，属于高频工作流，可以直接位于根命令下：

```bash
ecos configure
ecos menuconfig
ecos build
ecos clean
ecos flash
ecos monitor
```

不得为了缩短输入而同时维护 `ecos create` 和 `ecos project create` 两套正式接口。确有
迁移需求时，短命令只能作为有明确移除期限的兼容别名。

## 2. 通用语法

```text
ecos [--sdk NAME|VERSION|PATH] <command> [subcommand] [arguments] [options]
```

当前已实现的全局选项：

| 选项 | 状态 | 作用 |
| --- | --- | --- |
| `-h`, `--help` | 已实现 | 显示根帮助；命令组和子命令也必须提供局部帮助。 |
| `-V`, `--version` | 已实现 | 显示 Python `ecos` CLI 版本。 |
| `--sdk SELECTOR` | 已实现 | 仅覆盖本次命令使用的 SDK。 |

`--sdk` 当前必须写在一级命令之前：

```bash
ecos --sdk 3.0.0 toolchain status
ecos --sdk /path/to/sdk sdk current
```

以下形式不属于当前语法：

```bash
ecos toolchain --sdk 3.0.0 status
```

路径选择优先级固定为：命令行 `--sdk`、工程 pin、当前路径所属的源码 checkout、
`ECOS_SDK_HOME` 兼容变量、用户 active SDK、从源码入口识别出的 checkout。高优先级选择
无效时必须直接报告错误，不得静默使用低优先级 SDK。

## 3. 命令总览

### 3.1 当前已实现命令

| 命令 | 作用 |
| --- | --- |
| `ecos sdk register PATH` | 注册 SDK checkout 或 release 安装。 |
| `ecos sdk list` | 列出已注册 SDK 及 active SDK。 |
| `ecos sdk current` | 按固定优先级解析当前使用的 SDK。 |
| `ecos sdk use SELECTOR` | 设置用户级 active SDK。 |
| `ecos sdk pin SELECTOR` | 将 SDK 注册项固定到工程。 |
| `ecos sdk unregister NAME` | 删除 SDK 注册关系，不删除 SDK 文件。 |
| `ecos sdk doctor` | 检查所有注册项的路径和清单。 |
| `ecos toolchain detect` | 识别宿主平台并选择 SDK 锁定的工具链资产。 |
| `ecos toolchain status` | 检查工具链安装状态和编译器身份。 |
| `ecos toolchain install` | 下载或导入并安装锁定的工具链。 |
| `ecos project create EXAMPLE` | 从指定 Example 创建外部工程并写入工程元数据。 |
| `ecos project set-board BOARD` | 设置 Board，并用 Board 清单映射覆盖 Target。 |
| `ecos project set-target TARGET` | 设置 Target/SoC，并清空工程的 Board。 |
| `ecos validate` | 只读校验并解析 Project、Example、Board、Target 和 Component 清单。 |
| `ecos configure` | 生成统一 resolved project、CMake 和 Kconfig 派生配置。 |
| `ecos build` | 按工程中的 Board/Target 选择 SoC 构建规则并生成固件。 |
| `ecos flash` | 按 Board 烧录配置写入 `artifacts.json` 声明的固件。 |
| `ecos monitor` | 按 Board 串口配置打开跨平台串口监视器。 |
| `ecos completion SHELL` | 输出指定 Shell 的补全脚本。 |

### 3.2 3.0 计划命令

| 命令 | 作用 |
| --- | --- |
| `ecos project info` | 显示工程、SDK、Board、Target、profile 和派生状态。 |
| `ecos project set-profile PROFILE` | 修改工程配置 profile。 |
| `ecos board list` | 列出当前 SDK 提供的 Board。 |
| `ecos board describe ID` | 显示 Board 清单、Target、资源和能力。 |
| `ecos target list` | 列出当前 SDK 提供的 Target。 |
| `ecos target describe ID` | 显示 Target/SoC、ISA、ABI 和外设能力。 |
| `ecos example list` | 列出 Example 及其能力要求。 |
| `ecos example describe ID` | 显示 Example 来源、依赖和兼容条件。 |
| `ecos component list` | 列出 SDK Component。 |
| `ecos component describe ID` | 显示 Component 依赖和公开接口元数据。 |
| `ecos menuconfig` | 进入跨平台交互配置界面。 |
| `ecos clean` | 清理工程构建产物，不删除用户源码。 |
| `ecos doctor` | 检查工程、SDK、工具链、烧录器、串口和宿主环境。 |

资源类命令统一采用 `<resource> list` 和 `<resource> describe ID`，不得分别发明
`list-boards`、`show-target`、`component-info` 等近似命令。Board 和 Target 在内部是不同
资源；工程选择 Board 后由 Board 清单解析 Target，工程元数据不得保存相互冲突的两份选择。

## 4. SDK 命令组

### 4.1 注册 SDK

```bash
ecos sdk register PATH [--name NAME] [--kind checkout|release] [--activate] [--replace]
```

| 参数 | 作用 |
| --- | --- |
| `PATH` | SDK checkout 或 release 根目录。 |
| `--name NAME` | 指定注册名；默认由 SDK 清单确定。 |
| `--kind` | 注册类型，默认为 `checkout`。 |
| `--activate` | 注册成功后设为 active SDK。 |
| `--replace` | 替换已有同名注册。 |
| `--format text\|json` | 选择人类可读或机器可读输出。 |

注册只记录并校验路径，不复制 SDK 文件。删除注册也不删除对应目录。

### 4.2 查询和切换

```bash
ecos sdk list [--format text|json]
ecos sdk current [--project PATH] [--sdk SELECTOR] [--format text|json]
ecos sdk use SELECTOR [--format text|json]
ecos sdk pin SELECTOR [--project PATH] [--format text|json]
ecos sdk unregister NAME [--format text|json]
ecos sdk doctor [--format text|json]
```

`sdk use` 修改用户注册表中的 active 项，不修改 `PATH`。`sdk pin` 写入工程 pin，其优先级
高于 active SDK。`sdk current` 是查询命令，不得修改注册表或工程。

## 5. 工具链命令组

```bash
ecos toolchain detect [--format text|json]
ecos toolchain status [--prefix PATH] [--custom DIRECTORY] [--format text|json]
ecos toolchain install [--prefix PATH] [--cache-dir PATH] [--archive PATH]
                       [--force] [--dry-run] [--format text|json]
```

| 命令 | 网络访问 | 文件写入 |
| --- | --- | --- |
| `detect` | 否 | 否 |
| `status` | 否 | 否 |
| `install --dry-run` | 否 | 否 |
| `install` | 缓存或本地归档不可用时访问 | 是 |

`--custom` 只检查明确提供的工具链目录。`--archive` 使用本地归档，但仍执行清单要求的
SHA-256 校验。`--force` 允许替换无效安装；正常且匹配的安装默认复用。

更完整的工具链行为见 [toolchain-cli.md](toolchain-cli.md)。

## 6. 工程命令组设计

工程创建的正式接口为：

```bash
ecos project create EXAMPLE [--name NAME] [--path DIRECTORY]
                            [--board ID | --target ID] [--profile PROFILE]
                            [--dry-run] [--force] [--format text|json]
```

参数的职责：

| 参数 | 作用 |
| --- | --- |
| `EXAMPLE` | 用作工程初始源码的全局唯一 Example 名称，例如 `hello`。不得包含父级目录。 |
| `--name NAME` | 工程目录名和默认 CMake 工程名；默认使用 Example 名称。 |
| `--path DIRECTORY` | 工程的父目录，默认当前目录。 |
| `--board ID` | 创建时选择 Board，并从 Board 清单派生 Target。 |
| `--target ID` | 创建时只选择 Target/SoC；与 `--board` 互斥。 |
| `--profile PROFILE` | 选择初始配置 profile。 |
| `--dry-run` | 显示目标路径、Example 和元数据计划，不执行写入。 |
| `--force` | 允许执行明确列出的替换；默认不得覆盖用户源码。 |
| `--format text\|json` | 选择人类可读或机器可读输出。 |

示例：

```bash
ecos project create hello
ecos project create hello --board starrysky-l4
ecos project create hello --name my-app
ecos project create hello --name my-app --path ~/workspace --board starrysky-l4
ecos project create hello --target ysyx-2512
ecos --sdk 3.0.0 project create hello --name hello-demo
```

创建后可随时切换硬件选择：

```bash
ecos project set-board starrysky-l4
ecos project set-target ysyx-2512
```

这两个命令使用覆盖语义。`set-board` 将 `board` 写为规范 Board ID，并将 `target`
覆盖为该 Board 清单声明的 Target；`set-target` 将 `target` 写为指定 SoC ID，并将
`board` 清空为 `null`。因此工程元数据不会保留不兼容的 Board 与 Target 组合。

`project create` 不提供独立的空工程模式。需要从最小工程开始时使用 `hello`
Example，再在生成的工程中修改应用源码。Example 可以存放在 SDK 内部的分类目录中，
但创建时只使用其清单声明的 `name`，不得携带 `get_start/` 等父级目录。所有 Example
名称必须全局唯一，重名时 CLI 必须明确失败。

已选择硬件的工程可直接构建：

```bash
ecos build
```

`ecos build` 必须重新验证 Board 到 Target 的映射，随后从
`components/soc/<target>/CMakeLists.txt` 加载目标构建规则，并使用 Ninja 生成器。
StarrySky L4 当前生成 `build/retrosoc_fw.elf`、`.bin`、`.txt`、`.hex`、`.map`、`.size`
和 `compile_commands.json`；缺少 ELF、BIN、HEX、MAP、size 报告或编译数据库时即使
底层构建命令返回成功也视为失败。

创建工程必须完成以下工作：

1. 解析所选 SDK，但不复制 SDK、BSP 或工具链源码到工程。
2. 复制所选 Example 目录的内容。
3. 写入 `.ecos/project.yml`，保存 schema、Example、Board、Target、profile 和可移植的
   SDK pin。
4. 验证 Board、Target、Component 和 Example 的能力及依赖关系。
5. 仅在 `build/` 或 `.ecos/generated/` 中生成本机派生配置。

当前统一解析器已经接入 `validate`、`configure`、`menuconfig`、`build`、`flash` 和
`monitor`。它负责
校验 Project、Example、Board、Target 和 Component 清单，递归解析 Component 依赖，检查
Board 到 Target 映射、profile、能力、源码、头文件、内存、产物和工具链声明，并产生唯一
的 resolved project model。CMake 不再自行选择 Board、Component 或应用源码。

工程元数据不得记录某台主机的 SDK 绝对路径。正式工程应位于 SDK 安装目录之外。

`ecos init_project` 属于 2.x/迁移期名称。若 3.0 Python CLI 后续提供兼容入口，它只能转发
到 `ecos project create`，并必须打印弃用提示；旧 Shell 脚本不进入 3.0 安装包。

## 7. 高频工程命令设计

以下已实现命令默认使用当前目录中的工程，同时支持 `--project PATH`，以便 IDE、CI 和 AI
不依赖进程当前目录：

```bash
ecos validate [--project PATH] [--format text|json]
ecos configure [--project PATH] [--dry-run] [--format text|json]
ecos menuconfig [--project PATH]
ecos build [--project PATH] [--clean] [--format text|json]
ecos flash [--project PATH] [--device DIRECTORY] [--format text|json]
ecos monitor [--project PATH] [--port PORT] [--baudrate RATE]
             [--timeout SECONDS] [--expect TEXT] [--format text|json]
```

`ecos configure` 只写 `.ecos/generated/`，并原子维护以下完整文件集：

- `resolved-project.json`：唯一的已解析工程模型。
- `resolved-project.cmake`、`sdkconfig.cmake`：CMake 输入。
- `Kconfig`、`.config`、`sdkconfig.h`：Kconfig 输入和输出。
- `configuration.fingerprint`：当前解析和 Kconfig 配置指纹。

`ecos menuconfig` 使用 Kconfiglib 的 curses 终端界面，将用户选择持久化到
`.ecos/project.config`，退出界面后自动执行与 `ecos configure` 相同的派生配置生成流程。
该命令必须在交互式终端中运行；Windows 安装包会额外安装 `windows-curses`。

`ecos build` 每次先检查并按需更新上述配置，然后从磁盘重新读取
`resolved-project.json`。构建成功后写入 `build/artifacts.json`，记录 ELF、BIN、HEX 等
文件的路径、大小和 SHA-256，以及架构、ISA、ABI、入口、段布局、SDK、Board、Target、
工具链、源码指纹和配置指纹。`flash` 只接受摘要完整、配置未过期且符合当前 Board 的产物清单，
不会猜测固件文件名。

StarrySky L4 的 mass-storage provider 在 GNU/Linux、macOS 和 Windows 上由 Python 查找
卷标，也允许 `--device` 显式指定挂载目录。`monitor` 使用 PySerial 发现或打开串口；JSON
模式必须通过 `--timeout` 或 `--expect` 保证命令能够结束。

以下命令仍处于计划状态：独立的 `clean` 和工程级 `doctor`。当前清理入口为
`ecos build --clean`。

工程命令遵循以下语义：

- `validate` 和其他查询命令无副作用。
- `configure` 生成派生状态，不修改用户维护的应用源码。
- `build` 使用 CMake 和 Ninja，并通过统一 SDK/工具链解析器获得环境。
- `clean` 只能删除已识别工程内的构建产物。
- `flash` 和 `monitor` 不从未校验的工程文本拼接 Shell 命令。
- `flash` 不应隐式启动永久运行的监视器；需要组合时由明确选项或顺序命令完成。

## 8. 输出与退出码

已实现的 SDK、工具链和工程命令支持：

```bash
ecos sdk list --format json
ecos toolchain status --format json
ecos validate --format json
ecos build --format json
```

JSON 顶层固定包含：

```text
cli_version
schema_version
command
status
data
diagnostics
```

JSON 数据写入标准输出；日志、警告和下载进度写入标准错误。机器模式不得包含颜色控制符。
面向人的文本输出使用统一的 `【ECOS-INFO】`、`【ECOS-WARN】` 和 `【ECOS-ERR】` 前缀。

当前退出码：

| 退出码 | 含义 |
| --- | --- |
| `0` | 命令成功。 |
| `2` | 用法或参数错误。 |
| `3` | SDK、注册表、清单或其他配置错误。 |
| `4` | 不支持的宿主平台或架构。 |
| `5` | 工具链校验、解压或外部程序错误。 |
| `6` | 网络下载错误。 |

未来为能力不匹配或构建失败增加退出码时，必须同时更新 Python 枚举、本文、JSON 契约
测试和 IDE fixture，不能复用已有退出码表达不同语义。

## 9. 自动补全

当前支持：

```bash
ecos completion bash
ecos completion zsh
ecos completion fish
ecos completion powershell
```

安装器会生成四类补全文件，并根据用户选择或检测到的 Shell 配置对应启动文件。补全规则：

- 只能公开状态为“已实现”的 Python 命令。
- 新命令实现时，必须同时补全命令组、子命令、枚举值和常用选项。
- 计划命令、旧 `bin/` Shell 命令和内部测试参数不得出现在用户补全中。
- Bash、Zsh、Fish 和 PowerShell 必须提供等价命令集合。
- Windows CMD 不提供可编程补全，Windows 默认面向 PowerShell。

## 10. 命名与参数规范

新增命令必须遵守：

1. 一级分组使用资源名词单数，例如 `project`、`sdk`、`toolchain`、`board`。
2. 二级子命令使用小写动词，例如 `create`、`list`、`describe`、`install`。
3. 多单词命令或长选项使用 kebab-case，例如 `set-board`、`--dry-run`。
4. 相同语义使用相同名称；查询详情统一使用 `describe`，不混用 `show`、`info` 和
   `inspect`。`project info` 是当前工程汇总视图的特例。
5. 路径使用明确的 `--project`、`--path`、`--prefix` 或 `--cache-dir`，不得用含义不明的
   `--dir`。
6. 布尔开关默认采用安全行为；覆盖、删除、替换必须显式使用 `--force`。
7. 查询和验证命令不得写文件；修改类命令应提供 `--dry-run`。
8. 参数完整时不得强制交互；交互功能必须存在可用于 CI 的非交互形式。
9. 命令不得依赖 Bash、Unix 路径分隔符或 `shell=True`。
10. 用户可见命令和选项使用英文 ASCII；诊断文本可以本地化，但稳定诊断码不能变化。

## 11. 变更和验收规则

增加或修改一个正式命令时，提交必须同时完成：

1. 在 Python CLI 中注册命令、参数和局部 `--help`。
2. 使用统一 `SdkContext` 和公共业务模块，不在参数处理层复制解析逻辑。
3. 增加成功、参数错误、无效状态和 `--dry-run` 测试。
4. 对机器接口增加 JSON envelope、诊断码和退出码契约测试。
5. 更新 Bash、Zsh、Fish、PowerShell 补全及一致性测试。
6. 将本文中的状态从“计划中”改为“已实现”，并补充完整参数说明。
7. 涉及安装或迁移时同步更新 `docs/install.md` 和 SDK 3.0 开发边界。

命令只有在实现、帮助、补全、文档和测试一致时，才可以标记为“已实现”。
