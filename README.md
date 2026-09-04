# ECOS 嵌入式 SDK (ECOS Embedded SDK)

[English](README_EN.md) | 中文

**ECOS 嵌入式 SDK** 目前是专为ECOS 芯片及Starry Sky板卡（如 `StarrySkyC1`, `StarrySkyC2`, `StarrySkyL3`, `StarrySkyL3_1`）打造的开发套件，未来也会考虑兼容市面上的RV芯片。

SDK 3.0 引入了Driver和BSP层，将SoC和板卡定义分离开，方便用户更加自由的进行配置。采用python构建kconfig 图形化配置与 CMake/Ninja 构建系统，致力于提供高效、安全的嵌入式开发体验。同时替换默认工具链为Xpack 多平台交叉编译工具链，使得SDK可以在多平台进行工作（目前主要支持Windows和Linux）

---

## 目录
1. [环境依赖与安装](#环境依赖与安装)
2. [标准开发与测试流程 (必读)](#标准开发与测试流程-必读)
3. [第三方组件支持](#第三方组件支持)
4. [分支管理](#分支管理)
5. [致谢](#致谢)

---

### 1. 环境依赖与安装

#### 依赖项
- SDK 锁定的 xPack GNU RISC-V Embedded GCC 15.2.0-1
  (`riscv-none-elf-gcc`，安装脚本按宿主平台下载并校验)
- CMake 3.20 或更高版本（安装脚本锁定 `cmake==3.31.10` 并安装到 SDK 私有目录）
- Ninja（安装脚本锁定 `ninja==1.11.1.4` 并安装到 SDK 私有目录）
- GNU Make（仅旧版/第三方 AbstractMachine 兼容流程需要）
- `direnv` (推荐，用于 `testdir` 自动加载环境变量)
- Python 3.9 或更高版本

#### 快速安装
详细的安装路径、参数、离线安装、多版本和故障排查见
[docs/install.md](docs/install.md)。

运行 Python 安装器。在 GNU/Linux 上默认安装到
`~/.local/share/ecos/sdk/3.0.0`，并安装当前宿主对应的锁定工具链以及 CMake/Ninja：
```bash
python3 tools/install.py
```
安装器会读取 `tools/sdk-manifest.json` 中的 SDK `3.0.0` 标识，复制该清单并将安装结果
注册为 active SDK。它会识别 Bash、Zsh、Fish 或 PowerShell，安装对应的 `ecos` 自动
补全，并在该 Shell 的用户启动文件中维护带 `ECOS SDK` 标记的配置块。该配置块会添加
SDK CLI、CMake/Ninja 路径和补全，不会持久设置 `ECOS_SDK_HOME`。安装完成后按输出提示
重新加载配置或打开新终端。可以先用以下命令在不写文件、不联网的情况下检查完整安装计划：
```bash
python3 tools/install.py --dry-run
```

离线安装 xPack 工具链使用 `--archive <path>`；首次安装 Python/CMake/Ninja 依赖仍需
PyPI 或 pip 本地缓存。`--skip-toolchain` 只跳过交叉编译器，SDK
所需的 Python、CMake 和 Ninja 依赖仍会安装。使用
`--shell bash|zsh|fish|powershell` 可以覆盖自动识别结果，
`--shell-profile <path>` 可以指定启动文件，`--shell none` 可以禁用 Shell 配置。
`--prefix <path>` 表示版本目录的父目录，安装器会从 SDK 清单自动追加版本号；例如
`--prefix ~/ecos-sdks` 的实际安装路径是 `~/ecos-sdks/3.0.0`。
`--registration-name <name>` 可以指定注册名，`--no-activate` 保留当前全局 active SDK，
`--force` 强制重新部署 SDK、替换同名注册，并在未跳过工具链时重新安装工具链。
`--replace-registration` 和 `--force-toolchain` 可用于只强制对应部分。完整参数见
`python3 tools/install.py --help`。

3.0 安装包不会复制源码 `bin/` 中的 2.x Shell 命令。版本目录的 `bin/` 只包含生成的
Python `ecos` 启动器；帮助和自动补全也只公开已经迁移到 Python 的命令。当前已迁移
命令为 `sdk`、`project`、`build`、`toolchain` 和 `completion`。

工具链识别、状态检查和安装由 Python `ecos` CLI 统一提供：
```bash
ecos sdk register /path/to/checkout --name dev --activate
ecos sdk list
ecos sdk current
ecos sdk use 3.0.0
ecos sdk pin 3.0.0 --project /path/to/project
ecos sdk doctor
ecos project create hello --path ~/workspace
ecos toolchain detect
ecos toolchain status
ecos toolchain install
ecos toolchain status --format json
ecos completion bash
ecos completion zsh
ecos completion fish
ecos completion powershell
```

SDK 路径按固定优先级解析：单次命令的全局 `--sdk`、工程 pin、当前路径所属的源码
checkout、兼容变量 `ECOS_SDK_HOME`、注册表 active 项、源码入口 checkout。高优先级配置
存在但无效时会直接报错，不会静默切换到另一个版本。`ecos sdk unregister` 只删除注册
关系，不删除 SDK 文件。

使用 `ecos toolchain install --dry-run` 可以在不联网、不写文件的情况下查看安装计划；
使用 `--archive <path>` 可以导入已离线下载的官方归档。

---

### 2. 标准开发与测试流程 (必读)

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

### 3. 第三方组件支持

#### ECOS Core Runtime（错误与日志）
Core Runtime 是 SDK 3.0 工程的默认组件，提供统一的 `ECOS_ERR_*` 错误语义和无堆内存日志服务。BSP Console 初始化成功后自动成为日志输出后端；默认输出 ASCII、CRLF 且不启用 ANSI 颜色。

- **代码使用示例**：
  ```c
  #include "ecos/bsp/console.h"
  #include "ecos/log.h"

  int main(void) {
      ECOS_RETURN_ON_ERROR(bsp_console_init());
      ecos_log_set_level(ECOS_LOG_DEBUG);
      ECOS_LOGI("app", "设备启动成功");
      ECOS_LOGW("app", "检测到异常输入");
      ECOS_LOG_ERR("app", ECOS_ERR_IO, "初始化外设");
      return ECOS_OK;
  }
  ```

  `ECOS_RETURN_ON_ERROR()` 用于向调用者传播错误；需要统一清理时使用
  `ECOS_GOTO_ON_ERROR()`；不可恢复的应用错误可使用 `ECOS_PANIC_ON_ERROR()`
  记录错误后停机。三个宏都只会对结果表达式求值一次。

---

### 4. 分支管理

仓库当前采用“版本分支 + 产品分支”的方式管理长期演进：

- `1.0`：维护 `v1.0.0` 及后续 `1.x` 稳定修复
- `2.0`：当前主开发分支
- `starryskypi`：StarrySkyPi 产品线分支

完整规则见 [docs/branching.md](docs/branching.md)。

---

### 5. 致谢
感谢以下开发者对 ECOS 嵌入式 SDK 的代码贡献：

- [XHTimmo](https://github.com/XHTimmo)
- [maksyuki](https://github.com/maksyuki)
- [Krismile233](https://github.com/Krismile233)
- [FINALxxx](https://github.com/FINALxxx)
- 雪泥喵爪
- Ayana nana
- [myyerrol](https://github.com/myyerrol)
