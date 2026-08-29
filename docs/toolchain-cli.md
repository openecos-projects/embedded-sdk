# Python 工具链命令

SDK 3.0 的工具链识别、状态检查和安装由 Python `ecos` CLI 提供。查询命令不访问网络，
只有显式执行 `ecos toolchain install` 才可能下载工具链。

## 命令

```bash
# 识别宿主平台并查看锁定的 xPack 资产，不检查本地安装
ecos toolchain detect

# 检查默认工具链的安装、版本和目标三元组
ecos toolchain status

# 检查用户明确指定的自定义工具链
ecos toolchain status --custom /path/to/toolchain

# 下载、校验并安装默认工具链
ecos toolchain install

# 不联网、不写文件，只输出安装计划
ecos toolchain install --dry-run

# 从官方离线归档安装，仍会执行清单中的 SHA-256 校验
ecos toolchain install --archive /path/to/xpack-archive
```

`detect`、`status` 和 `install` 均支持 `--format json`。JSON 输出固定包含
`cli_version`、`schema_version`、`command`、`status`、`data` 和 `diagnostics`；下载和解压
进度只写入标准错误。

`xPack` 是清单中的 provider，不是一个名为 `xpack` 的命令。安装后的交叉编译器命令为
`riscv-none-elf-gcc`、`riscv-none-elf-objcopy` 等。安装器不会把版本化工具链目录加入全局
`PATH`，以避免多个 SDK 版本互相抢占；`ecos build` 使用状态检查返回的绝对路径。
CMake 和 Ninja 属于独立的宿主构建依赖，由 `tools/install.py` 以锁定版本安装到 SDK
私有目录；它们的路径会加入受管 Shell 配置，构建编排也会直接使用绝对路径。

源码 checkout 执行 `ecos toolchain install` 时，默认前缀是用户目录
`~/.local/ecos-sdk`；`tools/install.py` 部署 release SDK 时，工具链位于对应版本 SDK 根目录
下的 `toolchain/`。用 `ecos toolchain status --format json` 查看当前上下文实际使用的路径。

## 退出码

| 退出码 | 含义 |
| --- | --- |
| `0` | 命令成功 |
| `2` | 命令参数错误 |
| `3` | 清单或 SDK 配置错误 |
| `4` | 不支持的宿主平台或架构 |
| `5` | 工具链校验、解压或外部程序错误 |
| `6` | 网络下载错误 |

`status` 在工具链缺失或无效时仍返回 `0`，因为状态查询本身已经成功；具体状态和修复建议
位于 `data.installation.state` 与 `diagnostics`。后续构建和验证命令会把不可用工具链作为失败。
