# Default Board Build Test Plan

## Metadata

- id: build.default-board
- phase: phase-1-foundation
- category: build-testing
- source_requirement: `.test/doc/00-overview/automated-testing-scope.md` -> `第一阶段：基础必测` -> `构建测试` -> `各芯片和板级默认配置能够编译通过`
- status: drafting
- owner: sdk-test
- priority: p0
- automation_target: CI and local pre-submit
- workspace_root: `.test`

## Goal

验证 SDK 当前支持的各芯片和板级默认配置都能完成无交互构建，并生成预期固件产物。

该测试只验证“默认配置能否成功构建”，不验证运行时行为、外设功能和硬件交互。

## Scope

### In Scope

- 自动发现 SDK 支持的 board。
- 为每个 board 生成默认 Kconfig 配置。
- 使用默认配置执行完整构建。
- 检查构建退出码。
- 检查必要构建产物是否生成。
- 保存每个 board 的构建日志和结果摘要。
- C2 阶段覆盖安装版 SDK 中所有适配 C2 的模板工程。

### Out Of Scope

- Debug/Release 组合构建。
- 组件开关组合构建。
- 示例工程和 demo 全量构建。
- QEMU、仿真器或真实硬件运行。
- 烧录、串口日志、外设行为验证。
- 性能、镜像大小趋势和内存水位门禁。

## Current Repository Facts

- SDK 使用 Kconfig 和 Makefile 构建系统。
- README 建议 CI/脚本使用无头默认配置：
  - `$ECOS_SDK_HOME/tools/kconfig/build/conf --alldefconfig Kconfig`
  - `$ECOS_SDK_HOME/tools/kconfig/build/conf --syncconfig Kconfig`
- README 中的默认构建命令是 `make -j$(nproc)`。
- README 中说明构建产物位于 `build/`，典型产物包括：
  - `retrosoc_fw.elf`
  - `retrosoc_fw.hex`
  - `retrosoc_fw.bin`
- README 明确要求不要直接在 SDK 安装路径下污染 `templates/`，测试应在隔离工作区执行。
- 当前仓库未发现 `*defconfig*` 文件，因此本方案使用 Kconfig 的 `alldefconfig` 作为默认配置来源。
- 所有测试环境、运行脚本、测试配置、临时工程、日志和报告都必须放在 `.test` 目录下。

## Workspace Layout

所有自动化测试相关内容统一放在 `.test` 下：

```text
.test/
  config/      # 测试矩阵、board manifest、suite 配置
  scripts/     # 测试入口、preflight、runner、artifact checker
  work/        # 自动生成的隔离测试工程和临时构建目录
  reports/     # 测试日志、result.json、汇总报告和 CI artifact
  doc/         # 测试设计和执行方案文档
```

约束：

- 测试脚本不得写入 `.test` 之外的临时目录。
- 测试工程不得创建到 `/tmp`、`templates/`、`board/` 或 SDK 根目录。
- 测试报告和日志不得散落到仓库其他位置。
- SDK 源码根目录只作为 `ECOS_SDK_HOME` 被测试读取和引用。
- `.test/work` 和 `.test/reports` 下的生成内容默认不进入 Git 追踪。

## Board Discovery

### Initial Candidate Set

从 `board/` 目录发现板级目录。

当前可见候选 board：

- `StarrySkyC1`
- `StarrySkyC2`
- `StarrySkyL3`
- `StarrySkyL3_1`

### Eligibility Rules

一个 board 进入默认构建矩阵，至少需要满足：

- `board/<board>/Makefile` 存在。
- 存在构建所需启动文件和链接脚本。
- 可以通过 SDK 脚手架或测试工作区映射到一个 target 名称。

建议维护一份显式 board manifest，避免目录名和 CLI target 名称不一致导致自动发现误判。

建议文件：

```text
.test/config/boards.yaml
```

建议字段：

```yaml
boards:
  - id: starrysky-c1
    board_dir: board/StarrySkyC1
    target: c1
    status: active
    default_template: hello
  - id: starrysky-c2
    board_dir: board/StarrySkyC2
    target: c2
    status: active
    default_template: hello
  - id: starrysky-l3
    board_dir: board/StarrySkyL3
    target: l3
    status: active
    default_template: hello
  - id: starrysky-l3-1
    board_dir: board/StarrySkyL3_1
    target: l3_1
    status: active
    default_template: hello
```

## Execution Model

### Required Mode

必须使用 SDK 对用户公开的 CLI 创建测试工程，然后在测试工程中构建。

禁止测试脚本直接使用 `cp`、`rsync` 或手写文件搬运逻辑创建工程。工程创建流程必须等价于用户可以手动执行的命令。

当前 C2 默认构建测试对每个适配 C2 的模板使用：

```bash
ecos init_project <template> -name default_build_c2_<template> -target c2
```

原因：

- 符合 README 对开发和测试隔离的要求。
- 避免污染 `templates/`。
- 避免直接在 `board/` 目录留下构建产物。
- 更接近 SDK 用户真实使用路径。
- 用户可以在终端直接复现自动化测试的工程创建步骤。

## Test Workspace

所有构建都必须发生在 `.test/work` 下。

推荐路径：

```text
.test/work/default-board-build/<run-id>/<board-id>/
```

推荐报告路径：

```text
.test/reports/default-board-build/<run-id>/<board-id>/
```

运行脚本路径：

```text
.test/scripts/run-default-board-build.sh
.test/scripts/preflight.sh
.test/scripts/check-artifacts.sh
```

这些路径是本测试的唯一合法写入目标。除 `.test/doc`、`.test/config`、`.test/scripts` 中被明确纳入版本管理的文件外，`.test/work` 和 `.test/reports` 的生成内容不得进入 Git 追踪。

## Environment Requirements

### Required Tools

- `make`
- `python3`
- `riscv64-unknown-elf-gcc`
- `riscv64-unknown-elf-objcopy`
- `riscv64-unknown-elf-objdump`
- SDK Kconfig `conf` 工具
- `ecos` CLI

### Required Environment Variables

- `ECOS_SDK_HOME`: SDK 根目录，示例值为仓库根目录。

### Preflight Checks

测试入口脚本在执行矩阵前必须检查：

- `ECOS_SDK_HOME` 已设置且目录存在。
- `$ECOS_SDK_HOME/tools/kconfig/Kconfig` 存在。
- RISC-V 交叉编译工具链在 `PATH` 中可用。
- `make` 可用。
- `ecos` 可用。

## Per Board Steps

对每个 board 执行以下步骤。

### Step 1: Create Isolated Project

执行公开 CLI：

```bash
ecos init_project <template> -name default_build_<target> -target <target>
```

当前 C2 模板矩阵：

```text
asm_hello (asm, no Kconfig configure step)
coroutine_test
filesystem_test
gpio
hello
i2c_scan
minesweeper
shell_test
spi_flash_test
spi_st7735
spi_st7735_donut
st7789
```

模板列表来自安装版 SDK 中 `ecos init_project list` 显示为支持 C2 的模板，并在 `.test/config/boards.yaml` 中显式维护。

汇编模板没有 `alldefconfig` 目标，测试只执行工程创建和 `make` 编译。C 模板执行 `make alldefconfig` 后再编译。

### Step 2: Generate Default Configuration

在测试工程目录中执行：

```bash
make alldefconfig
```

`alldefconfig` 是 `menuconfig` 的无交互等价入口，由 Makefile 负责设置 board Kconfig、driver Kconfig、调用 Kconfig `conf`，并把生成物同步到项目 Makefile 实际使用的 `configs/` 目录。

通过条件：

- 命令退出码为 0。
- `.config` 或工程约定的配置输出存在。
- `configs/config/auto.conf` 存在，或 Makefile 实际包含的配置文件存在。

### Step 3: Build Firmware

执行：

```bash
make -j$(nproc)
```

通过条件：

- 命令退出码为 0。
- 构建日志中不存在 fatal error。
- 构建日志中不存在 `No such file or directory`、`undefined reference`、`Error 1` 等明确失败信号。

### Step 4: Check Artifacts

检查：

- `build/` 目录存在。
- 至少存在一个 ELF 产物。
- 至少存在一个二进制或 hex 产物。

优先检查 README 中声明的产物：

```text
build/retrosoc_fw.elf
build/retrosoc_fw.hex
build/retrosoc_fw.bin
```

由于部分 board Makefile 可能生成 `app.elf`、`loader.elf` 或无 `.elf` 后缀的 ELF 文件，初期建议采用兼容检查：

```text
build/*.elf OR file type is ELF
build/*.hex OR build/*.bin
```

当 board manifest 完善后，再为每个 board 固化预期产物清单。

### Step 5: Save Logs And Result

每个 board 保存到 `.test/reports/default-board-build/<run-id>/<board-id>/`：

- `configure.log`
- `build.log`
- `artifacts.txt`
- `result.json`

推荐结果格式：

```json
{
  "id": "starrysky-l3-1",
  "target": "l3_1",
  "template": "hello",
  "status": "pass",
  "configure_exit_code": 0,
  "build_exit_code": 0,
  "artifacts": [
    "build/retrosoc_fw.elf",
    "build/retrosoc_fw.hex",
    "build/retrosoc_fw.bin"
  ]
}
```

### Step 6: Cleanup

默认保留失败用例工作区，清理成功用例工作区。

建议策略：

- CI PR: 成功清理，失败保留日志和必要产物。
- Nightly: 保留完整日志和产物摘要，按周期清理旧 run。
- Local: 默认保留，便于开发者调试。

## Pass And Fail Criteria

### Pass

一个 board 通过需要同时满足：

- 默认配置生成成功。
- 构建成功。
- 必要固件产物生成。
- 结果文件写入成功。

整个测试通过需要：

- 所有 active board 均通过。

### Fail

以下任一情况视为失败：

- board manifest 中 active board 无法创建工程。
- Kconfig 默认配置生成失败。
- 构建命令返回非 0。
- 构建产物缺失。
- 测试脚本无法生成结果摘要。

### Skip

以下情况可以 skip，但必须在结果中写明原因：

- board 标记为 `experimental` 或 `deprecated`。
- board 缺少工具链支持。
- board 明确不支持当前默认 template。
- board 需要私有依赖或硬件专用资源才能构建。

## CI Integration

### PR Gate

PR 阶段建议运行最小矩阵：

- 所有 `active` board。
- 每个 board 只使用默认 template。
- 构建并检查产物。

### Nightly

Nightly 阶段可以扩展：

- 所有 active board。
- 更多 template。
- Debug/Release 组合。
- 产物大小统计。
- 构建耗时趋势。

### Report Summary

CI 输出应包含：

```text
board-id         target   template   configure   build   artifacts   status
starrysky-c1     c1       hello      pass        pass    pass        pass
starrysky-c2     c2       hello      pass        pass    pass        pass
starrysky-l3     l3       hello      pass        pass    pass        pass
starrysky-l3-1   l3_1     hello      pass        pass    pass        pass
```

## Implementation Tasks

建议后续按以下顺序落地：

1. 新增 `.test/config/boards.yaml`，显式描述 board 构建矩阵。
2. 新增 `.test/scripts/preflight.sh`，检查工具链、`ECOS_SDK_HOME` 和 Kconfig 工具。
3. 新增 `.test/scripts/run-default-board-build.sh`，执行逐 board 构建。
4. 新增 `.test/scripts/check-artifacts.sh`，统一检查 `build/*.elf` 和 `build/*.bin|*.hex`。
5. 新增 `.test/work/default-board-build/`，作为隔离测试工程根目录。
6. 新增 `.test/reports/default-board-build/`，保存本地和 CI 执行结果。
7. 将该测试接入 CI PR gate。

## Open Questions

- 当前所有 board 是否都支持 `hello` template？
- `StarrySkyL3` 和 `StarrySkyC1` 是否仍属于 active board？
- 是否需要在第一阶段把 `templates/` 中所有 target 也纳入 board discovery？
- Kconfig 工具是否需要测试脚本自动构建？

## Future Refinement

- 将兼容产物检查收紧为 board 级精确产物清单。
- 添加构建耗时和产物大小记录。
- 将 warning policy 接入构建测试。
- 将失败日志中的常见错误归类为工具链、配置、源码、链接或产物错误。
