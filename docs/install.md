# ECOS SDK 3.0 安装指南

`tools/sdk-manifest.json` 是 SDK 身份、版本和发布文件范围的来源。当前仓库的
`channel` 为 `development`，因此从本 checkout 运行 `python3 tools/install.py`
默认只注册源码树，不复制 SDK。带有 `release` 通道清单的正式制品才会部署到版本目录。

## 1. 开发 checkout

在仓库根目录执行：

```bash
python3 tools/install.py --dry-run --format json
python3 tools/install.py
ecos sdk current
```

开发模式把 checkout 注册为 `checkout`，源码修改立即生效。安装器还会在用户级目录安装
Python `ecos` 启动器、CLI 依赖、CMake、Ninja、wlink、Shell 补全，以及缺失时的锁定工具链。
`--dry-run` 不写文件也不访问网络。使用 `--skip-toolchain` 可暂缓交叉编译器安装；随后运行
`ecos toolchain install`。使用 `--skip-wlink` 可暂缓 wlink 烧录工具安装。

要注册另一个 checkout 而不运行安装器，可执行：

```bash
ecos sdk register /path/to/checkout --name dev --kind checkout --activate
```

## 2. 正式 release

发布制品须由匹配 `tools/sdk-manifest.json` 中 `sdk_version` 的 Git tag 生成，清单
`channel` 必须为 `release`。发布脚本只打包清单 `release.paths` 声明的 3.0 路径，并排除
`release.exclude` 列出的用户 BSP、测试和开发目录；未声明的工作树文件不会进入制品。

发布脚本同时生成归档对应的 SPDX 2.3 文件清单和锁定主机依赖清单
`ecos-embedded-sdk-<version>.spdx.json`；`SHA256SUMS` 包含 tar.gz、zip 和 SBOM 的摘要。
SBOM 中未完成审查的许可证标为 `NOASSERTION`，正式发布前仍需核对第三方许可证。

解压发布制品后，在其根目录执行：

```bash
python3 tools/install.py --dry-run
python3 tools/install.py
ecos sdk current
ecos toolchain status
```

release 模式把声明的 SDK 文件先复制到同一文件系统的 staging 目录，计算文件摘要，
校验后一次性激活 `<sdk_version>` 目录。同版本内容一致时复用已有安装；内容不同或旧
安装没有有效标记时拒绝覆盖，`--force` 也不会改写已有版本内容。其他 SDK 版本不会受影响。

默认正式版本目录：

| 平台 | 版本目录 |
| --- | --- |
| GNU/Linux | `${XDG_DATA_HOME:-~/.local/share}/ecos/sdk/<sdk_version>/` |
| macOS | `~/Library/Application Support/ECOS/SDKs/<sdk_version>/` |
| Windows | `%LOCALAPPDATA%\ECOS\SDKs\<sdk_version>\` |

`--prefix PATH` 指定版本目录的父目录，安装器仍会追加清单中的版本号。
`--mode development|release` 可明确选择模式；`--mode auto` 是默认值，按清单通道
选择。开发清单不能通过 `--mode release` 安装为正式版本。

## 3. 主机工具与用户数据

全局 `ecos` CLI 不绑定某个 SDK 版本。GNU/Linux 和 macOS 默认入口是
`~/.local/bin/ecos`，Windows 默认入口是 `%LOCALAPPDATA%\ECOS\bin\ecos.cmd`。
CLI 包、Python 依赖、CMake、Ninja、wlink 和补全安装在主机级目录；切换 active SDK 只修改
注册表，不重写 `PATH`。可通过 `--host-prefix PATH` 指定主机工具前缀。

交叉编译器按 `<toolchain-id>/<release>/<host>` 共享安装：

| 平台 | 默认工具链根目录 |
| --- | --- |
| GNU/Linux | `${XDG_DATA_HOME:-~/.local/share}/ecos/toolchains/` |
| macOS | `~/Library/Application Support/ECOS/toolchains/` |
| Windows | `%LOCALAPPDATA%\ECOS\toolchains\` |

当前 SDK 锁定 xPack GNU RISC-V Embedded GCC `15.2.0-1`，编译器命令前缀是
`riscv-none-elf-`。`ecos build` 通过工具链状态解析绝对路径，不要求把交叉编译器加入
全局 `PATH`。下载缓存独立于安装目录；清理缓存不会删除已安装工具链。

wlink 同样按 `<release>/<host>` 安装在主机级目录：

| 平台 | 默认 wlink 根目录 |
| --- | --- |
| GNU/Linux | `~/.local/lib/ecos/tools/wlink/` |
| macOS | `~/.local/lib/ecos/tools/wlink/` |
| Windows | `%LOCALAPPDATA%\ECOS\lib\ecos\tools\wlink\` |

当前锁定 wlink `0.1.3`。`ecos flash` 默认使用该受管安装；`--tool PATH` 或
`ECOS_WLINK` 可覆盖。当前 pin 提供 `linux-x86_64`、`macos-arm64` 和
`windows-x86_64` 资产；其他宿主会跳过 wlink 安装，需要手动提供可执行文件。
Windows 受管资产 `wlink-v0.1.3-win-x64.zip` 使用 nusb/WinUSB 后端，需要 WCH-Link
调试接口绑定 WinUSB（复合设备还需正确的 USBCCGP 父驱动），不能直接使用 WCH WDM
驱动 `WCHLink_A64`。给该 x64 程序补 `CH375DLL64.dll` 不能解决驱动不兼容问题。

如果需要保留 WCH-LinkUtility/MRS 使用的 WDM 驱动，可自行下载支持原生 WCH 驱动的
[x86 wlink 0.1.3](https://github.com/ch32-rs/wlink/releases/download/v0.1.3/wlink-v0.1.3-win-x86.zip)，
保留发行包文件并按其要求准备 WCH DLL，用明确路径覆盖受管程序：

```powershell
ecos flash --tool C:\tools\wlink-x86\wlink.exe
# 或在当前 PowerShell 会话设置
$env:ECOS_WLINK = "C:\tools\wlink-x86\wlink.exe"
ecos flash
```

仅把另一个 wlink 放到 `PATH` 不会覆盖已有的受管安装。遇到
`incompatible driver is installed for this interface` 时，CLI 返回配置诊断
`ECOS_FLASH_DRIVER_INCOMPATIBLE`，提示使用兼容的 x86 工具或为调试接口安装 WinUSB；
不会自动换工具、重绑驱动或继续擦写。若选择改绑 WinUSB，请先确认 USB 接口，勿替换
串口接口驱动；更换调试接口驱动可能影响 WCH-LinkUtility/MRS。

用户 BSP 存在版本目录之外：

| 平台 | 默认用户 BSP 目录 |
| --- | --- |
| GNU/Linux | `${XDG_DATA_HOME:-~/.local/share}/ecos/boards/UserBSP/` |
| macOS | `~/Library/Application Support/ECOS/boards/UserBSP/` |
| Windows | `%LOCALAPPDATA%\ECOS\boards\UserBSP\` |

从旧安装升级时，安装器会把版本目录中原有的 `boards/UserBSP` 文件复制到该目录；同名
内容冲突时停止，不覆盖用户文件。CLI 在 SDK Board 目录和用户 BSP 目录中发现板卡与
组件。`ECOS_USER_BSP_ROOT` 可覆盖用户 BSP 路径，安装与 CLI 运行时须使用相同值。

主机级 `share/ecos/cli-launch.json` 记录 Python 可执行文件和 `bin/ecos` 脚本路径，
供 VS Code 等客户端在 Windows 上以参数数组、无 Shell 的方式启动 CLI。

## 4. 依赖与离线安装

需要 Python 3.9 或更高版本以及可运行的 `python -m pip`。安装器把锁定的
`PyYAML`、JSON Schema、Kconfiglib、PySerial、CMake 和 Ninja 安装到主机级私有目录；首次安装这些
wheel 时需要 PyPI 或已有的 pip 缓存。Windows 另需 `windows-curses`。

工具链覆盖 GNU/Linux x86_64/arm64、macOS x86_64/arm64 和 Windows x86_64。
使用已取得的官方归档可跳过工具链下载，但仍会验证 SHA-256、宿主和编译器身份：

```bash
python3 tools/install.py --archive /path/to/xpack-archive.tar.gz
ecos toolchain detect
ecos toolchain status --format json
```

`--archive` 只影响交叉工具链，不提供 Python wheel。若只想先配置 SDK 和主机工具：

```bash
python3 tools/install.py --skip-toolchain
ecos toolchain install
```

wlink 也可离线安装或暂缓：

```bash
python3 tools/install.py --wlink-archive /path/to/wlink-v0.1.3-linux-x64.tar.gz
python3 tools/install.py --skip-wlink
```

`--skip-wlink` 之后可设置 `ECOS_WLINK` 或 `--tool` 指向手动准备的 wlink。

### 更新已安装 CLI 的内置清单

全局 `ecos` 使用主机级安装中的 Python 包和工具链清单；修改 SDK checkout 中的 JSON
不会自动更新该副本。更新到修复 CH32V103 ISA/ABI 声明的代码后，应从此 checkout
重跑安装器以刷新 CLI（PowerShell 将 `python3` 换为 `python`）：

```bash
python3 tools/install.py --skip-toolchain --skip-wlink --shell none
```

该命令仍会检查/安装主机 Python、CMake/Ninja 依赖并注册 SDK；不会重装交叉工具链和
wlink，也不修改 Shell 配置。若有同名 SDK 注册冲突，确认目标 checkout 后添加
`--replace-registration`。不要只修改已安装目录里的 JSON 作为长期修复。

## 5. 注册表与多版本

默认注册表为 GNU/Linux `${XDG_CONFIG_HOME:-~/.config}/ecos/sdks.json`、macOS
`~/Library/Application Support/ECOS/sdks.json`、Windows `%APPDATA%\ECOS\sdks.json`。
`--registry PATH` 可在安装时覆盖该路径；CLI 使用 `ECOS_SDK_REGISTRY` 环境变量。
注册表采用带 schema 版本的 JSON、锁文件和原子替换。

```bash
ecos sdk list
ecos sdk current
ecos sdk use 3.0.0
ecos sdk pin 3.0.0 --project /path/to/project
ecos sdk doctor
ecos sdk unregister 3.0.0
```

`unregister` 只移除注册关系，不删除 checkout 或正式安装目录。SDK 选择优先级为单次
`--sdk`、项目 pin、`ECOS_SDK_HOME` 兼容变量、active 注册项、源码入口 checkout。
高优先级选择无效时会明确失败，不回退到另一个版本。`ECOS_SDK_HOME` 不应持久写入
Shell 启动文件。

从旧 `~/.local/ecos-sdk` 安装迁移时，同名注册可能仍指向旧目录。先运行
`python3 tools/install.py --dry-run --format json` 查看冲突；`--replace-registration`
只替换注册关系，`--force` 还会强制重装主机依赖和工具链，但不会覆盖不同内容的正式
SDK 版本目录。旧目录不会自动删除。

## 6. Shell 与补全

安装器识别 Bash、Zsh、Fish、PowerShell，在对应的用户启动文件中维护
`# >>> ECOS SDK >>>` 与 `# <<< ECOS SDK <<<` 标记块。它加入主机级 `ecos`、
CMake、Ninja 路径，并加载补全；不把某个版本的 SDK 或工具链目录加入全局 `PATH`。

```bash
python3 tools/install.py --shell zsh
python3 tools/install.py --shell bash --shell-profile /path/to/bashrc
python3 tools/install.py --shell none
ecos completion bash
```

安装结束后打开新终端，或重新加载对应启动文件。`--shell none` 不修改启动文件。

## 7. 验证与移除

```bash
ecos sdk current --format json
ecos sdk doctor --format json
ecos toolchain status --format json
ecos doctor --project /path/to/project --format json
```

移除正式版本前先确认没有工程 pin 使用它，并预览准确路径：

```bash
ecos sdk uninstall 3.0.0 --dry-run --format json
ecos sdk uninstall 3.0.0
```

卸载命令只删除带有效安装标记、内容摘要未改变且没有其他注册项引用的 release 版本目录，
并移除对应注册关系。若它是 active SDK，注册表的 active 项会清空。checkout、用户 BSP、
共享工具链、下载缓存和其他版本目录不会被删除。目录已有修改时需先人工核对，再单独处理。

机器模式的成功退出码为 `0`；用法、配置、能力不匹配、外部工具及网络错误分别使用
`2`、`3`、`4`、`5`、`6`。自动化程序应读取 `--format json` 的 `status`、`data` 和
`diagnostics`。
