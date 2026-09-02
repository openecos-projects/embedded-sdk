# ECOS SDK 3.0 安装指南

本文说明 ECOS Embedded SDK 3.0 当前 Python 安装器的行为、安装路径、工具链处理、SDK
注册、多版本切换和常见故障。文中的安装命令应从 SDK 源码仓库根目录执行。

当前 SDK 版本由 `tools/sdk-manifest.json` 唯一确定。安装器不接受手工指定 SDK 版本的
参数，也不会根据 Git 分支名、目录名或 Python CLI 版本推断 SDK 版本。

## 1. 安装结果概览

默认安装命令：

```bash
python3 tools/install.py
```

安装器依次完成：

1. 读取并校验 SDK 清单和必要目录。
2. 从清单读取 `sdk_version`，计算平台对应的版本目录。
3. 选择当前主机对应的 xPack RISC-V 工具链资产。
4. 复制 SDK 文件，但不复制源码 `bin/` 中的旧 Shell 命令。
5. 将锁定的 Python、CMake 和 Ninja 依赖安装到 SDK 私有目录，已正确安装时直接复用。
6. 检测工具链；缺失时下载、校验并安装，已正确安装时直接复用。
7. 将最终路径写入用户级 SDK 注册表，并默认设为 active SDK。
8. 生成 Python `ecos` 启动器和四类 Shell 补全文件。
9. 幂等更新当前 Shell 的用户启动文件。

当前 `3.0.0` 在 GNU/Linux 上默认安装到：

```text
~/.local/share/ecos/sdk/3.0.0/
```

安装输出中的 `SDK install base` 是版本目录的父目录；`SDK installed at` 才是本次安装的
实际 SDK 根目录。

`tools/install.py` 安装的是 release SDK，工具链和 CMake/Ninja 依赖随 release 放在上述
版本目录下。若后续仍从源码 checkout 运行 `python3 tools/ecos.py --sdk . ...`，该上下文
默认使用独立的用户级工具链前缀；请额外执行
`python3 tools/ecos.py --sdk . toolchain install`，或改用已安装 release 的 `ecos` 入口。
源码 checkout 的构建工具会优先使用 checkout 中的私有依赖，否则使用宿主机 PATH；
release SDK 则直接使用安装器部署的 CMake/Ninja。两种情况下都不要求把交叉编译器加入
全局 `PATH`。

## 2. 前置条件与支持平台

### 2.1 必要条件

- Python 3.9 或更高版本，并可运行 `python -m pip`。
- 能够读取当前 SDK 源码目录并写入用户数据目录。
- 默认安装需要访问 PyPI 和 GitHub Release；安装器不会调用系统包管理器。
- 离线安装时提供与当前主机和锁定清单匹配的官方归档。

### 2.2 当前工具链资产覆盖

- GNU/Linux x86_64
- GNU/Linux arm64
- macOS x86_64
- macOS arm64
- Windows x86_64

不在工具链清单中的系统或架构会在下载前失败，并报告不支持的宿主平台。

## 3. 版本化安装路径

### 3.1 平台默认路径

安装器遵循平台用户数据目录，并自动追加清单中的 `<sdk_version>`：

| 平台 | 安装父目录 | 当前实际目录 |
| --- | --- | --- |
| GNU/Linux | `${XDG_DATA_HOME:-~/.local/share}/ecos/sdk` | `.../ecos/sdk/3.0.0` |
| macOS | `~/Library/Application Support/ECOS/SDKs` | `.../ECOS/SDKs/3.0.0` |
| Windows | `%LOCALAPPDATA%\ECOS\SDKs` | `...\ECOS\SDKs\3.0.0` |

GNU/Linux 设置了 `XDG_DATA_HOME` 时，安装器优先使用该目录。

### 3.2 `--prefix` 的含义

`--prefix` 表示版本目录的父目录，不是最终 SDK 目录：

```bash
python3 tools/install.py --prefix ~/ecos-sdks
```

当前版本的实际结果为：

```text
~/ecos-sdks/3.0.0/
```

不要把 `3.0.0` 再写入 `--prefix`，否则会得到
`~/ecos-sdks/3.0.0/3.0.0`。注册名也不会改变磁盘版本目录。

同一个父目录可以并存多个版本：

```text
~/ecos-sdks/
├── 3.0.0/
├── 3.1.0/
└── 4.0.0-beta.1/
```

## 4. 推荐安装流程

### 4.1 先检查计划

```bash
python3 tools/install.py --dry-run
```

`--dry-run` 不创建目录、不修改注册表和 Shell 文件，也不会访问网络。它会显示：

- SDK 源码路径和最终版本目录。
- 将要复制的目录与文件。
- 当前主机匹配的工具链资产。
- 将要安装的 Python、CMake 和 Ninja 依赖及其 SDK 私有路径。
- 将要写入的注册名和 active 状态。
- 将要修改的 Shell 启动文件。

如果已有同名注册指向其他路径，dry-run 不会因此中断。它会继续输出计划，在
`data.registration.requires_force` 标记 `true`，并给出
`ECOS_SDK_REGISTRATION_CONFLICT` 警告。真实安装仍需要显式使用 `--force` 或
`--replace-registration`。

CI 或其他自动化工具可以使用 JSON：

```bash
python3 tools/install.py --dry-run --format json
```

JSON 写入标准输出，日志和下载进度写入标准错误。

### 4.2 执行并验证

```bash
python3 tools/install.py
```

安装完成后，根据输出重新加载 Shell，例如：

```bash
source ~/.bashrc
```

也可以关闭并重新打开终端。随后检查：

```bash
ecos sdk current
ecos sdk list
ecos toolchain status
```

## 5. 安装器参数

完整帮助：

```bash
python3 tools/install.py --help
```

| 参数 | 作用 |
| --- | --- |
| `--prefix PATH` | 指定版本目录的父目录，安装器自动追加清单版本号。 |
| `--cache-dir PATH` | 覆盖工具链下载缓存目录。 |
| `--archive PATH` | 使用本地官方交叉工具链归档，不下载 xPack。 |
| `--skip-toolchain` | 跳过交叉编译器；Python、CMake 和 Ninja 依赖仍会安装。 |
| `--force-toolchain` | 只强制重装工具链，不替换 SDK 注册冲突。 |
| `--force` | 重新部署 SDK、替换同名注册，并在启用工具链时强制重装工具链。 |
| `--dry-run` | 输出完整计划，不写文件、不联网。 |
| `--format text\|json` | 选择文本或机器可读 JSON。 |
| `--shell auto\|bash\|zsh\|fish\|powershell\|none` | 自动识别、明确选择或禁用 Shell 配置。 |
| `--shell-profile PATH` | 覆盖自动选择的 Shell 启动文件。 |
| `--registry PATH` | 覆盖注册表路径，主要用于隔离环境和测试。 |
| `--registration-name NAME` | 覆盖注册名；默认使用 SDK 版本。 |
| `--no-activate` | 注册安装结果，但不改变当前 active SDK。 |
| `--replace-registration` | 只替换同名注册，不强制重装工具链。 |

以下组合无效：

- `--skip-toolchain --archive PATH`
- `--skip-toolchain --force-toolchain`
- `--shell none --shell-profile PATH`

`--force --skip-toolchain` 是有效组合：强制部署 SDK 和替换注册，但跳过交叉编译器。

## 6. 安装内容

安装器复制以下目录：

```text
components/  hal/          templates/    environments/
third_party/ board/        example/      docs/
devices/     tools/fixdep/ tools/kconfig tools/scripts/
tools/toolchains/          tools/ecos_cli/
```

同时复制：

```text
tools/ecos.py
tools/sdk-manifest.json
```

安装器还会在版本目录中生成受管的主机依赖：

```text
lib/ecos/python/                 # CLI 的 PyYAML、Kconfiglib 和 PySerial 依赖
lib/ecos/host/bin/ninja          # Ninja（Windows 为 Scripts/ninja.exe）
lib/ecos/host/cmake/data/bin/    # CMake 原生可执行文件
```

当前锁定版本为 `PyYAML==6.0.3`、`kconfiglib==14.1.0`、`pyserial==3.5`、
`cmake==3.31.10` 和 `ninja==1.11.1.4`。CMake/Ninja
使用 Python wheel 安装，不写入系统 Python 或系统级程序目录。

安装器不会复制源码 `bin/`。版本目录中的 `bin/` 会重新生成，并且只包含：

```text
bin/ecos
bin/ecos.cmd
```

所以旧的 `ecos-board`、`ecos-init_project` 等 Shell 脚本不会进入 3.0 安装结果或补全。

当前安装器更新同一版本时会保留版本目录中的 `board/UserBSP` 用户内容，其余受管理 SDK
目录按本次源码重新部署。正式工程应放在 SDK 目录之外。

## 7. 交叉编译工具链

### 7.1 锁定版本与目录

SDK 3.0.0 当前要求：

```text
xPack GNU RISC-V Embedded GCC 15.2.0-1
命令前缀: riscv-none-elf-
目标三元组: riscv-none-elf
```

release SDK 的默认工具链目录位于对应版本目录内：

```text
<sdk-root>/toolchain/versions/xpack-riscv-none-elf-gcc/15.2.0-1/
<sdk-root>/toolchain/riscv -> 当前激活的工具链目录
```

### 7.2 已安装检测

每次安装前都会检测工具链。判定 `installed` 必须同时满足：

1. `bin/riscv-none-elf-gcc`（Windows 为 `.exe`）存在。
2. 执行 `-dumpmachine` 得到 `riscv-none-elf`。
3. 执行 `--version` 得到与锁定 Release 匹配的 GCC 版本。
4. `.ecos-toolchain.json` 中的 ID、Release、宿主和归档 SHA-256 与清单一致。

| 状态 | 安装行为 |
| --- | --- |
| `installed` | 默认直接复用，不下载、不解压。 |
| `missing` | 从缓存或网络取得归档并安装。 |
| `invalid` | 默认停止，避免覆盖未知内容。 |

修复无效工具链：

```bash
python3 tools/install.py --force-toolchain
```

强制重装整个 SDK 和工具链：

```bash
python3 tools/install.py --force
```

### 7.3 下载缓存

| 平台 | 默认缓存目录 |
| --- | --- |
| GNU/Linux | `${XDG_CACHE_HOME:-~/.cache}/ecos/toolchains` |
| macOS | `~/Library/Caches/ecos/toolchains` |
| Windows | `%LOCALAPPDATA%\ECOS\Cache\toolchains` |

已有缓存会先验证 SHA-256。缓存损坏时，在线安装会删除该归档并重新下载；本地
`--archive` 校验失败时会直接报错。

工具链查询命令：

```bash
ecos toolchain detect
ecos toolchain status
ecos toolchain status --format json
```

`detect` 只显示锁定资产；`status` 才检查本地安装。

## 8. 离线和分步安装

从其他环境取得 `ecos toolchain detect` 所列出的准确归档，然后执行：

```bash
python3 tools/install.py --archive /path/to/xpack-archive.tar.gz
```

Windows 示例：

```powershell
python tools/install.py --archive C:\Downloads\xpack-riscv-none-elf-gcc-15.2.0-1-win32-x64.zip
```

离线归档仍必须通过 SHA-256、归档根目录和编译器可执行性校验。`--archive` 只覆盖
xPack 交叉工具链；首次安装 Python、CMake 和 Ninja 依赖时，pip 仍需访问 PyPI 或使用
已经填充的本地缓存。

先安装 SDK、稍后安装工具链：

```bash
python3 tools/install.py --skip-toolchain
ecos toolchain install
```

## 9. 注册表与多版本

### 9.1 注册表路径

| 平台 | 默认注册表 |
| --- | --- |
| GNU/Linux | `${XDG_CONFIG_HOME:-~/.config}/ecos/sdks.json` |
| macOS | `~/Library/Application Support/ECOS/sdks.json` |
| Windows | `%APPDATA%\ECOS\sdks.json` |

注册表是带 schema 版本的 JSON，更新时使用进程间锁和原子替换。安装器默认以
`sdk_version` 作为注册名，并将结果登记为 `release`。

安装器可以通过 `--registry PATH` 使用隔离注册表。SDK CLI 也支持环境变量覆盖：

```bash
ECOS_SDK_REGISTRY=/path/to/sdks.json ecos sdk list
```

### 9.2 管理命令

```bash
# 列出注册项，星号表示 active
ecos sdk list

# 显示当前解析结果
ecos sdk current

# 修改全局 active SDK
ecos sdk use 3.0.0

# 将已注册 SDK 固定到工程
ecos sdk pin 3.0.0 --project /path/to/project

# 检查注册路径
ecos sdk doctor

# 只删除注册关系，不删除 SDK 文件
ecos sdk unregister 3.0.0
```

注册开发 checkout，不复制源码：

```bash
python3 tools/ecos.py sdk register . --name dev --kind checkout --activate
```

注册名只能包含字母、数字、点、下划线和连字符，并以字母或数字开头。

### 9.3 解析优先级

资源型命令按以下顺序选择 SDK：

1. 当前命令的 `--sdk NAME|VERSION|PATH`。
2. 从工程向上找到的 `.ecos/sdk.json` pin。
3. 当前路径所属的源码 checkout。
4. 兼容变量 `ECOS_SDK_HOME`。
5. 注册表 active SDK。
6. 从源码 `tools/ecos.py` 运行时推导的 checkout。

单次使用指定 SDK：

```bash
ecos --sdk 3.0.0 toolchain status
python3 tools/ecos.py --sdk . toolchain detect
```

高优先级选择无效时会直接报错，不会静默回退到另一个版本。
如果旧版启动文件仍设置了 `ECOS_SDK_HOME`，它会按兼容覆盖规则优先选择旧 SDK；可先执行
`Remove-Item Env:ECOS_SDK_HOME`（PowerShell）或 `unset ECOS_SDK_HOME`（Bash/Zsh），再用
`ecos sdk current` 检查实际上下文。

## 10. Shell 配置和补全

| Shell | 默认启动文件 |
| --- | --- |
| Bash | `~/.bashrc` |
| Zsh | `~/.zshrc` |
| Fish | `~/.config/fish/config.fish` |
| PowerShell | `$PROFILE.CurrentUserCurrentHost`；查询失败时使用用户 Documents 下的 PowerShell profile |

安装器维护以下标记之间的内容：

```text
# >>> ECOS SDK >>>
...
# <<< ECOS SDK <<<
```

重复安装会更新同一标记块。它会添加版本目录 `bin` 以及 SDK 私有 CMake/Ninja 目录到
`PATH` 并加载补全，不持久设置 `ECOS_SDK_HOME`，也不修改标记块之外的配置。工具链的
`toolchain/riscv/bin` 不会加入全局 `PATH`：`xPack` 是工具链提供者名称，实际编译器命令是
`riscv-none-elf-gcc`。`ecos build` 会从工具链状态解析器取得绝对路径，因此不要求用户
手工设置交叉编译器 `PATH`；需要手工调用时请使用 `toolchain/riscv/bin` 下的完整路径，或
只在当前 Shell 临时追加该目录。

常用方式：

```bash
python3 tools/install.py --shell zsh
python3 tools/install.py --shell fish
python3 tools/install.py --shell powershell
python3 tools/install.py --shell bash --shell-profile /path/to/bashrc
python3 tools/install.py --shell none
```

单独生成补全：

```bash
ecos completion bash
ecos completion zsh
ecos completion fish
ecos completion powershell
```

## 11. 更新、覆盖和旧路径迁移

### 11.1 正常重复安装

相同版本、相同注册路径下可以重复运行安装器。SDK 受管理文件会重新部署，有效工具链会
直接复用，Shell 配置和注册表更新保持幂等。

### 11.2 旧注册路径冲突

从旧的 `~/.local/ecos-sdk` 迁移时可能看到：

```text
【ECOS-ERR】 [ECOS_SDK_REGISTRY_INVALID] SDK registration '3.0.0'
already identifies /home/user/.local/ecos-sdk (3.0.0)
```

可以先查看不会写入文件的迁移计划：

```bash
python3 tools/install.py --dry-run
```

此命令会报告冲突和所需选项，但不会因注册冲突提前终止。

强制重装并把注册更新到新目录：

```bash
python3 tools/install.py --force
```

只允许替换同名注册、但不强制重装已有的有效工具链：

```bash
python3 tools/install.py --replace-registration
```

强制部署 SDK、但跳过工具链：

```bash
python3 tools/install.py --force --skip-toolchain
```

旧目录不会因注册替换而自动删除。确认新安装和工程正常后，再单独处理旧目录。

### 11.3 active 与项目版本

安装新版本默认将其设为 active。保留当前 active：

```bash
python3 tools/install.py --no-activate
```

之后可以显式切换：

```bash
ecos sdk list
ecos sdk use 3.0.0
ecos sdk current
```

项目 pin 的优先级高于全局 active，因此全局切换不会改变已固定版本的工程。

## 12. 常见故障

### 12.1 `ecos: command not found`

按安装输出重新加载 Shell，或者打开新终端：

```bash
source ~/.bashrc
```

确认版本目录的 `bin/ecos` 存在，并检查启动文件中的 `ECOS SDK` 标记块。使用
`--shell none` 时安装器不会修改 `PATH`。

### 12.2 `ECOS_SDK_HOME` 指向旧目录

该变量优先于 active 注册项。清除当前会话变量：

```bash
# Bash / Zsh
unset ECOS_SDK_HOME

# Fish
set -e ECOS_SDK_HOME
```

PowerShell：

```powershell
Remove-Item Env:ECOS_SDK_HOME -ErrorAction SilentlyContinue
```

随后执行：

```bash
ecos sdk current
ecos sdk doctor
```

### 12.3 工具链为 `invalid`

```bash
ecos toolchain status --format json
python3 tools/install.py --force-toolchain
```

不要手工修改 `.ecos-toolchain.json` 绕过版本或摘要校验。

### 12.4 下载中断或缓存损坏

重新运行安装器即可。下载使用临时 `.part` 文件，完成后再原子替换缓存；已有缓存会重新
验证 SHA-256。持续失败时，可取得 `ecos toolchain detect` 指定的官方归档，再使用
`--archive` 离线安装。

### 12.5 注册路径被移动或删除

```bash
ecos sdk doctor
ecos sdk unregister dev
python3 tools/ecos.py sdk register /new/path/to/sdk --name dev --activate
```

`unregister` 只修改注册表，不删除 SDK 或工程文件。

## 13. 安装后的最低验证

```bash
ecos sdk list
ecos sdk current
ecos sdk doctor
ecos toolchain detect
ecos toolchain status
```

机器可读验证：

```bash
ecos sdk current --format json
ecos sdk doctor --format json
ecos toolchain status --format json
```

期望结果：

- 当前 SDK 指向清单版本对应的版本化目录。
- `sdk doctor` 报告注册项为 `valid`。
- `toolchain status` 报告 `state: installed`。
- 编译器路径位于当前 release SDK 的工具链版本目录。
- 安装结果中的 `host_dependencies.installation.state` 为 `installed` 或 `reused`，且包含
  可运行的 CMake 和 Ninja 路径。

## 14. 移除安装

当前安装器尚未提供删除 SDK 文件的命令。`ecos sdk unregister` 只删除注册关系，不删除
版本目录。移除某个版本前应先确认它不是仍在使用的 active SDK 或工程 pin：

```bash
ecos sdk list
ecos sdk use another-version
ecos sdk unregister 3.0.0
```

然后核对注册表输出和目标路径，再通过操作系统文件管理方式删除准确的版本目录。不要删除
安装父目录，因为其中可能还有其他 SDK 版本。如果 Shell 配置仍引用被删除版本的 `bin`，
应先安装并激活另一个版本，让安装器更新 `ECOS SDK` 标记块。

## 15. 退出码

| 退出码 | 含义 |
| --- | --- |
| `0` | 成功。 |
| `2` | 命令参数错误。 |
| `3` | SDK 清单、路径、注册表或安装配置错误。 |
| `4` | 不支持的宿主系统或架构。 |
| `5` | 工具链校验、解压、外部程序或文件操作失败。 |
| `6` | 网络下载失败。 |

自动化环境应优先解析 `--format json` 的 `status`、`data` 和 `diagnostics`，不要依赖彩色
文本或下载进度行。
