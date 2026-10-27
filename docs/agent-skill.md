# ECOS SDK Agent Skill

SDK 3.x 提供一个标准 `SKILL.md` 格式的官方 Skill：
[`.agents/skills/ecos-sdk/SKILL.md`](../.agents/skills/ecos-sdk/SKILL.md)。
源码与 SDK 同仓维护，并通过 `tools/sdk-manifest.json` 的发布文件清单随 SDK 分发；
也可以独立安装到用户或应用工程的 Skill 目录。**普通 `tools/install.py` 不会自动安装
Skill，也不会修改任何 Agent 配置。**

## 能做什么

- 查询当前 SDK、Board/Target、Example、Component 与能力，创建及构建应用工程。
- 使用公共 Driver/Device/BSP API，避免混入旧版 HAL 或私有寄存器接口。
- 规范 HAL/Driver/Board/Device 适配，检查 Manifest、资源、示例、测试和文档配套。
- 排查构建、产物、Windows wlink/串口及中断问题。
- 生成并分析实机 Checklist，区分编译/寄存器模型/CI 与硬件验收证据。

Skill 是按任务加载的工作流程，不是新编译器、硬件模拟器、权限系统或保证正确的自动驾驶。
它不会凭空增加 Target capability，也不能替代资料核对、真实接线和测量。
烧录、解除保护、擦写、看门狗、低功耗和驱动改绑仍须明确授权。

## 目录与加载方式

```text
.agents/skills/ecos-sdk/
├── SKILL.md
├── LICENSE
├── references/
│   ├── project-workflow.md
│   ├── porting-workflow.md
│   ├── troubleshooting.md
│   └── hardware-validation.md
└── scripts/
    └── check_project.py
```

`SKILL.md` 只放入口、路由和必须遵守的边界，任务细节放在 references；API 和实际能力
以**选中的 SDK** 的文档、头文件、CLI 和 Manifest 为准，不在 Skill 复制另一份支持表。
仓库级 `AGENTS.md` / `.agent/guide.md` 等仍然适用；Skill 不覆盖更高优先级指令。

不同 Agent 的发现目录/显式调用方法不同，标准格式并不代表所有客户端都自动支持。
例如 Pi 能发现 `.agents/skills/` 和 `~/.agents/skills/`，可以用 `/skill:ecos-sdk` 显式加载，
修改或安装后用 `/reload` 或重启。在 SDK 仓库中工作时通常可直接发现；在外部应用工程中
工作则需要客户端显式配置或下面的安装步骤。请先审阅 Skill 和脚本，再授予项目信任。
其他客户端请按其官方 Skill 目录设置，不要未经确认覆盖 `AGENTS.md`、`CLAUDE.md`
或客户端 `settings.json`。

Pi 中可先执行 `/reload`，再按任务显式调用，例如：

```text
/skill:ecos-sdk 查询当前 SDK，为指定 Board 创建并构建 hello，不执行烧录。
/skill:ecos-sdk 为这个 Target 增加外设后端，先核对公共接口和芯片资源，再列出适配计划。
/skill:ecos-sdk 分析这份 Windows 烧录/串口日志，给出证据和下一步检查，不改绑驱动。
```

这些是给 Agent 的任务，不是 `ecos` CLI 命令。自动路由失败时用显式调用，确认它已经
读取 Skill；未经用户授权不得因为任务提到硬件就自动执行硬件操作。

## 显式安装（离线，无第三方 Python 依赖）

脚本仅复制 `ecos-sdk` Skill，不安装 CLI/工具链、不联网、不改注册表/系统 PATH/Agent
settings，也不接触硬件。需 Python ≥ 3.9。Windows 把 `python3` 换成 `python` 即可。

必须选择一个安装作用域，没有默认全局安装：

```bash
# 项目级：复制到 /path/to/app/.agents/skills/ecos-sdk
python3 tools/install_skill.py --project /path/to/app --dry-run --format json
python3 tools/install_skill.py --project /path/to/app

# 用户级（影响支持此发现目录的所有 Agent 项目）：~/.agents/skills/ecos-sdk
python3 tools/install_skill.py --user --dry-run
python3 tools/install_skill.py --user

# 自定义：--skills-dir 是父目录，脚本会追加 ecos-sdk
python3 tools/install_skill.py --skills-dir /path/to/client/skills --dry-run
python3 tools/install_skill.py --skills-dir /path/to/client/skills
```

PowerShell 示例（SDK 与应用不必在同一磁盘）：

```powershell
python C:\ECOS\embedded-sdk\tools\install_skill.py --project "D:\projects\my-device" --dry-run --format json
python C:\ECOS\embedded-sdk\tools\install_skill.py --project "D:\projects\my-device"
```

项目目录必须已存在。脚本默认从自身所属 SDK 复制 Skill，`--sdk-root PATH` 可显式选择
复制来源；这不是应用工程的 SDK 选择参数。SDK checkout 自身已有源码 Skill，无需再安装
到自身；脚本会拒绝源与目标重叠、符号链接/junction、未受管的同名目录以及用户改动过的已安装内容。
选择自定义目录时，不要传 Skill 目录本身，否则会多追加一层 `ecos-sdk`。

安装行为：

| 目标状态 | 结果 |
| --- | --- |
| 不存在 | 新建 Skill，写 `.ecos-skill-install.json` 文件摘要标记 |
| 受管、未修改且内容相同 | 复用，不重新写文件 |
| 受管、未修改但 SDK 中 Skill 已更新 | 默认拒绝；审阅后显式 `--update` |
| 未受管，或已安装 Skill 被修改/增删文件 | 拒绝覆盖，即使 `--update` 也不强制清除 |
| `--dry-run` | 同样执行校验，仅输出计划，不建目录/写文件 |

更新：

```bash
python3 tools/install_skill.py --project /path/to/app --update --dry-run
python3 tools/install_skill.py --project /path/to/app --update
```

安装器先在同一文件系统 staging 复制校验，再替换；替换失败时尝试恢复旧目录。
这不是对不可信的并发文件系统修改的防护沙箱；不要在 Agent/编辑器修改已安装 Skill 的
同时更新。安装标记记录的是 **Skill 分发来源**，不是应用使用哪个 SDK 的选择配置。

若需要自定义 Skill，先备份到另一个名称/目录，再自行管理。卸载没有自动命令：确认路径和
用户修改后，只删除明确安装的 `ecos-sdk` 目录，不要删除整个 `.agents` 或其他 Skill。
如果通过客户端显式配置加载，还需由用户自行移除对应配置项。重复安装到多个发现目录
可能造成同名冲突，保留一个明确来源即可。

## SDK 外如何定位事实来源

安装在应用或用户目录的 Skill **不使用自身相对路径猜 SDK**，也不会内嵌开发者机器的路径。
它先调用：

```bash
ecos sdk current --project /path/to/app --format json
# 用户明确指定某个 SDK 时：--sdk 必须在一级命令之前
ecos --sdk /path/to/sdk sdk current --project /path/to/app --format json
```

确认 `root/sdk_id/sdk_version/channel` 后，读取该 root 的 `tools/sdk-manifest.json`，后续用
明确的 `--sdk` 和 `--project`。Skill 适用于 `ecos-embedded-sdk` major 3，SDK manifest schema 1、
CLI major 3 / JSON schema 1.0；其他 major/schema 先停止并阅读对应 SDK 规范。代码分支和 CLI 包版本
不代替 SDK 清单版本；开发分支还应记录 commit。

用户选择/pin 无效时报告错误，不静默切 SDK、不修改 pin/active 注册项来绕过问题。新版本
需要新的流程时，从对应 SDK 更新 Skill；API 签名和能力仍由运行时选中的 SDK 查询。

## 可选的只读工程检查助手

`check_project.py` 是独立 Python 脚本，不是新增 `ecos` 子命令。它只运行四类只读 CLI
命令：`sdk current`、`project info`、`validate`、`toolchain status`，把原始 JSON 结果和
失败诊断组合成报告，**不会 configure/build/flash/monitor/安装或自动修复**。

```bash
python3 /path/to/skill/scripts/check_project.py --project /path/to/app
python3 /path/to/skill/scripts/check_project.py --sdk /path/to/sdk --project /path/to/app

# 显式提供 CLI 路径（使用当前 Python 和它已有的 CLI 依赖）
python3 /path/to/skill/scripts/check_project.py --cli /path/to/sdk/tools/ecos.py --sdk /path/to/sdk --project /path/to/app
```

在 Windows 默认安装中，`--cli` 可指定 `%LOCALAPPDATA%\ECOS\bin\ecos.cmd`。脚本通过其旁边
的 `share/ecos/cli-launch.json` 调用真实 Python/script，使用参数数组而不是 `shell=True`。
自定义 BAT 文件不受支持；源码 `tools/ecos.py` 方式要求该 Python 已能导入 SDK CLI 依赖。

- CLI 查询不写应用配置/构建目录；子进程关闭 Python 字节码写入，不打开串口或探针。
- 超时可通过 `--timeout <秒>` 设置，默认每条命令 60 秒；不做无限等待。
- 工具链状态命令退出码为 0，但 installation state 不是 `installed` 时，助手仍返回错误。
- 助手退出码 0 表示检查成功，3 表示选择/诊断失败，stdout 为 JSON；参数用法错误为 2，
  由 argparse 输出帮助/错误到 stderr，不保证生成 JSON。
- 报告中的 `hardware_tested: false` 永远不代表硬件失败，只表示该脚本没有做实机测试。
- JSON 含绝对路径/CLI 诊断，向外分享前需检查敏感信息。

## 维护与验证

- Skill 与 SDK API/CLI/安全边界一起审查；新能力以 Manifest 和兼容 Example 作为事实源。
- 发布范围仅包含 `.agents/skills/ecos-sdk` 和 `tools/install_skill.py`，不打包其他 Agent 私有状态。
- 测试覆盖格式/引用、SDK 分发复制、离线 dry-run/安装/复用/更新/冲突/恢复、路径安全以及
  只读助手在外部项目和 Windows CMD descriptor 下的行为。
- 宿主测试使用 `testdir/` 临时目录，不需要硬件。运行方式：

```bash
python3 -m unittest discover -s tools/ecos_cli/tests -p 'test_agent_skill.py' -v
```

Agent 客户端是否成功发现/路由 Skill 还需在对应客户端验证；格式和脚本测试通过不等于
每个 Agent 都有相同的自动加载/权限行为。首次使用可显式调用，确认它先查询 SDK，而不是
直接猜板卡、生成私有寄存器代码或执行烧录。
