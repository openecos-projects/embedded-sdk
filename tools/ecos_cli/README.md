# ECOS Python CLI

该目录保存 SDK 3.0 Python `ecos` CLI 的包配置、源码、工具链清单和测试。
命令层级、已实现能力和 3.0 计划接口见
[`docs/cli.md`](../../docs/cli.md)。

从仓库直接运行：

```bash
python3 tools/install.py --dry-run
python3 tools/ecos.py toolchain detect
python3 tools/ecos.py toolchain status --format json
python3 tools/ecos.py toolchain install --dry-run
python3 tools/ecos.py project list
python3 tools/ecos.py board list --format json
python3 tools/ecos.py target describe ysyx-2512-1 --format json
python3 tools/ecos.py component describe driver-gpio --format json
python3 tools/ecos.py schema describe board --format json
python3 tools/ecos.py schema validate board boards/StarrySkyL3_1/ecos-board.yml
python3 tools/ecos.py project create hello --path ~/workspace
python3 tools/ecos.py project create hello --name my-app --path ~/workspace
python3 tools/ecos.py project info --project ~/workspace/my-app --format json
python3 tools/ecos.py project set-profile memory --project ~/workspace/my-app
python3 tools/ecos.py project create hello --target ysyx-2512-1 --path ~/workspace
python3 tools/ecos.py project create hello --path ~/workspace --dry-run
python3 tools/ecos.py project set-board starrysky-l4-c1 --project ~/workspace/hello
python3 tools/ecos.py project set-target ysyx-2512-1 --project ~/workspace/hello
python3 tools/ecos.py validate --project ~/workspace/hello
python3 tools/ecos.py configure --project ~/workspace/hello
python3 tools/ecos.py menuconfig --project ~/workspace/hello
python3 tools/ecos.py build --project ~/workspace/hello
python3 tools/ecos.py clean --project ~/workspace/hello --dry-run
python3 tools/ecos.py doctor --project ~/workspace/hello --format json
python3 tools/ecos.py flash --project ~/workspace/hello
python3 tools/ecos.py monitor --project ~/workspace/hello --port /dev/ttyUSB0
python3 tools/ecos.py completion bash
python3 tools/ecos.py completion zsh
python3 tools/ecos.py completion fish
python3 tools/ecos.py completion powershell
```

`tools/install.py` 会自动识别当前 Shell，将四类补全文件安装到
`share/ecos/completions/`，并以 `ECOS SDK` 标记块幂等配置对应的用户启动文件。
安装器还会把锁定的 `PyYAML`、JSON Schema、Kconfiglib、PySerial、CMake 和 Ninja wheel 安装到
主机级私有依赖目录，构建时优先使用这些受管工具，不依赖系统级 CMake/Ninja 或交叉编译器 PATH。
可用 `--shell` 覆盖识别结果、`--shell-profile` 指定启动文件或用 `--shell none`
禁用配置。

安装器从 `tools/sdk-manifest.json` 读取版本号和模式。当前仓库为 `development` 通道，
默认注册 checkout 而不复制 SDK；正式 `release` 制品才部署到版本目录。
在 release 模式下，`--prefix` 是版本目录的父目录，实际安装目标自动追加清单版本号。
完整路径、共享工具链及用户 BSP 布局见 [`docs/install.md`](../../docs/install.md)。

主机级 `bin/ecos` 与 `bin/ecos.cmd` 由 Python 安装器生成；旧版 Shell 工程入口已从
源码树移除，也不会出现在 3.0 帮助或自动补全中。

构建或安装 Python 包时，以本目录作为项目根目录：

```bash
python3 -m pip install ./tools/ecos_cli
```
