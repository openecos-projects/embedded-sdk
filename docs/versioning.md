# SDK 3.x 版本管理与发布规则

## 1. 当前状态与 SemVer

`v3.1` 从 `3.0.1` 的 `59bcb77`（RTC 备份域时钟选择修复）分出，完整继承第一阶段
CH32V103/C8T6 适配和 ECOS SDK Agent Skill，不重写 `3.0.1` 的提交历史。
新增 Target、Board、公共 Driver/HAL 和示例属于向后兼容的功能扩展，应按 SemVer
发布为 **MINOR `3.1.0`**，不能仅因工作分支名为 `3.0.1` 就作为 PATCH 发布。

当前开发身份：

| 项目 | 值 / 来源 |
| --- | --- |
| 开发分支 | `v3.1` |
| SDK 版本 | `3.1.0-dev.0`，唯一来源为 `tools/sdk-manifest.json` |
| 通道 | `development` |
| Python CLI 包版本 | `3.1.0.dev0`，`tools/ecos_cli/pyproject.toml` 和 `tools/ecos_cli/src/ecos_cli/__init__.py` 必须一致 |
| CLI 兼容性 | major `3`，JSON schema `1.0`；本轮不升级协议 |

SDK 使用 SemVer 预发布形式 `-dev.0`，Python 包使用 PEP 440 的 `.dev0`；两者不能机械
复制。CLI 是独立的主机工具，SDK 身份不能从 CLI 包版本或 Skill 安装路径推断。
历史测试 fixture、`3.0.0` 发布记录和 `3.0.1` 实机记录保留原版本，不做全仓替换。

## 2. 分支与不可变边界

| 分支 / 标签 | 职责 |
| --- | --- |
| `v3.0.0` | 已发布标签，不移动、不重新发布不同内容 |
| `main`、`v3.0` | 保留已有 3.0 基线；本轮迁移不修改 |
| `3.0.1` | 保留第一阶段适配历史，仅接收维护修复；新增功能转到 `v3.1` |
| `v3.1` | 3.1 开发线，维持 `development` 通道 |
| `release/3.1.0-rc.1` | 后续按需创建的候选发布分支，本次不创建 |
| `v3.1.0-rc.1`、`v3.1.0` | 后续通过发布门槛后创建的不可变 annotated tags |
| `3.1.0` | 正式发布后才从 `v3.1.0` 创建的维护分支 |

不得 force-push、rebase 或移动已有发布标签来完成版本迁移。`3.0.1` 的维护修复如需进入
3.1，应在 `v3.1` 单独合入并回归，不把新功能反向灌入维护线。
CI 的 push / pull_request 触发分支保留 `main`、`v3.0`、`3.0.1`，并新增 `v3.1`。
标签发布工作流仍使用 `v*` 触发，不能绕过版本 / 通道校验。

## 3. 开发验证与证据边界

所有本地编译工程和产物必须放在 `testdir/` 下，配置使用无头命令，不运行交互式
`menuconfig`。测试应使用隔离的 `ECOS_SDK_REGISTRY`，不要为了通过测试改写用户全局
active SDK、工程 pin 或工具链配置。

版本迁移至少检查：

- CLI 完整回归、SDK manifest / schema 校验，以及 Python 包版本一致性；
- CH32V103 的 29 个兼容示例创建、无头构建、ELF / ISA / ABI / 容量契约；
- 发布路径完整性，确认 CH32 Target/Board、公共 Driver/HAL、示例、Skill 与其显式
  安装器进入 SDK 包，用户 BSP、测试工程和内部资料不进入制品；
- 开发通道不能作为正式版本发布，RC / 正式标签必须与清单精确匹配；
- 在临时仓库内预演 SDK 归档、摘要、SBOM 和归档安装器检查，不在真实开发分支上创建
  “测试发布”标签，也不把预演制品当作正式交付。

本机旧注册项可能仍记录 `3.0.0`，切换分支后显式路径选择也会因身份不一致而报错。
这是安全检查，不是版本解析失败；手动刷新方式见
[安装指南：从旧开发版本切换](install.md#从旧开发版本切换到-v31)。

CH32 第一阶段范围保持关闭；本次版本迁移**不增加实机验收结论**。
[Windows 实机清单](ch32v103-windows-test-checklist.md) 仍是 `3.0.1` 阶段的历史证据：

- 编译、CI、ELF 检查、寄存器模型、初始化 smoke 和硬件测量是不同证据；
- RTC 备份域修复已进入基线，但 `rtc-counter` 修复后的实机复测仍待完成；
- DMA/Flash/PWR/USB 的 smoke 结果不等于传输、擦写、低功耗或枚举验收；
- 不宣称 CAN、DMA2、TouchKey、I2C2、ADC2、TIM3/TIM4、完整 HSE/PLL 切换、USB class
  或外设集成 DMA API 已受支持；标准 SPI 继续使用 `hal/spi`、`drivers/spi`。

## 4. 后续 RC 流程（本次不执行）

在 `v3.1` 通过回归并完成候选范围评审后，再创建候选分支：

```bash
git switch -c release/3.1.0-rc.1 v3.1
```

同步候选身份：

```json
"sdk_version": "3.1.0-rc.1",
"channel": "release"
```

若同步 CLI 包版本，两处设置为 PEP 440 的 `3.1.0rc1`；兼容 major 和 schema 不变。
提交候选清单前完成回归、文档和包内容检查，再创建标签：

```bash
git add tools/sdk-manifest.json tools/ecos_cli/pyproject.toml \
  tools/ecos_cli/src/ecos_cli/__init__.py
# 同时提交经过评审的当前版本文档更新，不使用 git add . 混入本地资料。
git commit -m "chore(release): prepare v3.1.0-rc.1"
git tag -a v3.1.0-rc.1 -m "Release v3.1.0-rc.1"
python3 tools/release.py --tag v3.1.0-rc.1 --ref v3.1.0-rc.1 \
  --output-dir testdir/release-3.1.0-rc.1
# 仅在评审和验证完成后发布：
git push -u origin release/3.1.0-rc.1
git push origin v3.1.0-rc.1
```

发布脚本从标签指向的已提交文件打包；工作区未提交修改不会进入归档。候选通道也必须
是 `release`，GitHub 工作流会因标签含 `-` 而标记 prerelease。

## 5. 正式发布与维护（本次不执行）

RC / 硬件范围评审通过后，在候选发布线上把 SDK 身份改为 `3.1.0`，保持
`channel: release`，同步 CLI 包两处为 `3.1.0`，更新当前安装示例及发布说明。
不要把未完成的硬件测试改标为通过。

```bash
git add tools/sdk-manifest.json tools/ecos_cli/pyproject.toml \
  tools/ecos_cli/src/ecos_cli/__init__.py
# 同时提交经过评审的当前版本文档和发布说明。
git commit -m "chore(release): prepare v3.1.0"
git tag -a v3.1.0 -m "Release v3.1.0"
python3 tools/release.py --tag v3.1.0 --ref v3.1.0 \
  --output-dir testdir/release-3.1.0
(cd testdir/release-3.1.0 && sha256sum -c SHA256SUMS)
git push origin HEAD
git push origin v3.1.0
```

发布的必要条件：

1. 标签去掉 `v` 后与 `sdk_version` 完全一致，且 `channel` 为 `release`。
2. `--ref` 必须解析为标签指向的提交。
3. `.github/workflows/release.yml` 的宿主 / SDK 矩阵、测试、归档安装与代表固件构建通过。
4. 制品包括 `.tar.gz`、`.zip`、SPDX 2.3 `.spdx.json`、`SHA256SUMS`，并关联 Artifact
   Attestation。已有同版本制品摘要不同时拒绝替换。

正式发布后才建立维护分支，并补充其 CI 分支触发配置：

```bash
git switch -c 3.1.0 v3.1.0
git push -u origin 3.1.0
```

`v3.1` 后续继续开发时重新使用 `development` 通道和下一开发版本，不移动任何已发布标签。
