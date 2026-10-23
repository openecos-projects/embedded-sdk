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

安装器会在主机级入口目录生成 Python `ecos` 启动器；SDK 源码树不再包含旧版 Shell
命令。帮助和自动补全只公开 3.0 Python CLI 命令。
当前公开入口包括 `sdk`、`toolchain`、`project`、资源查询、`schema`、
`validate`、`configure`、`menuconfig`、`build`、`clean`、`doctor`、
`flash`、`monitor` 和 `completion`；完整命令表见
[CLI 文档](docs/cli.md)。

工具链识别、状态检查和安装由 Python `ecos` CLI 统一提供：
```bash
ecos sdk register /path/to/checkout --name dev --activate
ecos sdk list
ecos sdk current
ecos sdk use 3.0.0
ecos sdk pin 3.0.0 --project /path/to/project
ecos sdk doctor
ecos project list
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

3.0 工程由 Python `ecos` CLI 创建和编排，核心构建使用 CMake/Ninja。不要在 SDK
安装目录或 `examples/` 源码中修改应用源码；工程目录应位于 SDK 外部。

#### 步骤 1：创建你的独立工程目录
得益于 `ecos` 命令和环境变量的配置，你可以在电脑的任意位置创建自己的项目。
```bash
# 假设你在自己的开发工作区
cd ~/my_workspace/
```

#### 步骤 2：创建工程
从 SDK Example 创建工程，并在创建时选择 Board：
```bash
ecos project list --format json
ecos project create hello --name my_smoke_test --path ~/my_workspace \
    --board starrysky-l3-1
cd my_smoke_test
```

#### 步骤 3：校验、配置和构建
```bash
ecos validate --project ~/my_workspace/my_smoke_test --format json
ecos configure --project ~/my_workspace/my_smoke_test
ecos build --project ~/my_workspace/my_smoke_test --format json
```
构建产物和 `artifacts.json` 位于工程的 `build/` 目录。切换 Board 时修改工程元数据后
重新执行 `ecos configure` 和 `ecos build`，不需要复制板卡专用应用源码。

CI 的 Ubuntu 全量构建会遍历每个 Board 支持的全部 Example，并上传
`ecos-firmware-matrix-<commit>` 产物。压缩包和其中的 `firmware/` 目录包含按
`<example>-<board>.bin` 命名的固件、`manifest.json` 和 `SHA256SUMS`；清单记录实际
profile、ISA、ABI、大小、摘要以及源码/配置指纹，自动化测试可以据此选择并校验固件。
本地可用同一命令复现：`python tools/ci_host_smoke.py --all-examples --firmware-dir ci-firmware`。

硬件操作使用同一 CLI：

```bash
ecos flash --project ~/my_workspace/my_smoke_test
ecos monitor --project ~/my_workspace/my_smoke_test --port /dev/ttyUSB0
```

---

### 3. 第三方组件支持

#### ECOS Core Runtime（错误与日志）
Core Runtime 和 libc 子集是 SDK 3.0 工程的默认组件。Core Runtime 提供统一的 `ECOS_ERR_*` 错误语义和无堆内存日志服务，libc 提供裸机编译所需的内存操作。BSP Console 初始化成功后自动成为日志输出后端；默认输出 ASCII、CRLF 且不启用 ANSI 颜色。

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
- `main`：默认分支，集成已发布的 SDK 版本
- `v3.0`：SDK 3.0 版本维护分支（原 `3.0-dev`）
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
