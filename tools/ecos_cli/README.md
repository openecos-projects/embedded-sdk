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
python3 tools/ecos.py project create hello --path ~/workspace
python3 tools/ecos.py project create hello --name my-app --path ~/workspace
python3 tools/ecos.py project create hello --target ysyx-2512 --path ~/workspace
python3 tools/ecos.py project create hello --path ~/workspace --dry-run
python3 tools/ecos.py project set-board starrysky-l4 --project ~/workspace/hello
python3 tools/ecos.py project set-target ysyx-2512 --project ~/workspace/hello
python3 tools/ecos.py validate --project ~/workspace/hello
python3 tools/ecos.py configure --project ~/workspace/hello
python3 tools/ecos.py build --project ~/workspace/hello
python3 tools/ecos.py flash --project ~/workspace/hello
python3 tools/ecos.py monitor --project ~/workspace/hello --port /dev/ttyUSB0
python3 tools/ecos.py completion bash
python3 tools/ecos.py completion zsh
python3 tools/ecos.py completion fish
python3 tools/ecos.py completion powershell
```

`tools/install.py` 会自动识别当前 Shell，将四类补全文件安装到
`share/ecos/completions/`，并以 `ECOS SDK` 标记块幂等配置对应的用户启动文件。
安装器还会把锁定的 `PyYAML`、Kconfiglib、PySerial、CMake 和 Ninja wheel 安装到
版本目录的私有依赖目录，
构建时优先使用这些 SDK-local 工具，不依赖系统级 CMake/Ninja 或交叉编译器 PATH。
可用 `--shell` 覆盖识别结果、`--shell-profile` 指定启动文件或用 `--shell none`
禁用配置。

安装器从 `tools/sdk-manifest.json` 读取版本号。`--prefix` 是版本目录的父目录，实际安装
目标会自动追加版本号；当前 `--prefix ~/ecos-sdks` 对应 `~/ecos-sdks/3.0.0`。不传参数时，
GNU/Linux、macOS 和 Windows 分别使用平台用户数据目录下的 `ecos/sdk` 或 `ECOS/SDKs`。

安装器不复制源码仓库的 `bin/`。安装目录下的 `bin/ecos` 与 `bin/ecos.cmd` 由 Python
安装器生成；2.x 的 `ecos-board`、`ecos-init_project` 等 Shell 脚本不会进入安装结果，也
不会出现在 3.0 帮助或自动补全中。

构建或安装 Python 包时，以本目录作为项目根目录：

```bash
python3 -m pip install ./tools/ecos_cli
```
