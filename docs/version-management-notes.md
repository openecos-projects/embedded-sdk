# 版本管理建议

本文记录当前仓库状态，并给出一套适合接手维护时使用的版本管理流程。

## 当前状态

- 远程仓库：`https://github.com/openecos-projects/embedded-sdk.git`
- 默认分支：`main`
- 当前 `main` 提交：`eb84d50`，已打标签 `v2.0.0`
- 当前维护分支：`2.0`
- 下一阶段开发分支：`2.1-dev`
- 旧版维护分支：`1.0`
- 产品分支：`starryskypi`

截至 2026-06-21 观察到的重要分支差异：

- `origin/2.0` 比 `origin/main` 多 1 个提交。
- `origin/2.1-dev` 比 `origin/2.0` 多 25 个提交。
- `origin/1.0` 是独立的旧稳定版本线，不应该大范围合并 `2.0` 的改动。

仓库目前没有 `.github/` CI 配置，没有 `CHANGELOG.md`，也没有统一的版本文件。

## 推荐的分支职责

- `main`：只作为公开发布入口，保持在最新稳定发布版本。
- `2.0`：维护 `2.0.x` 系列的稳定版本。
- `2.1-dev`：集成下一个小版本的开发内容。
- `1.0`：旧稳定版本维护分支，只接收必要的关键修复。
- `starryskypi`：产品专用分支，只在确有需要时从版本分支同步改动。

## 日常开发流程

新工作应该从对应职责的分支切出：

```bash
git fetch origin
git switch 2.0
git pull --ff-only origin 2.0
# 创建对应的修复分支开始开发
git switch -c fix/2.0/short-topic
```

需要进入 `2.0.x` 发布的修复，放在 `2.0` 上做。

不适合进入 `2.0.x` 补丁版本的新功能，放在 `2.1-dev` 上做。

避免直接提交到 `main`。

## 提交信息风格

仓库现有提交基本已经在使用 Conventional Commit 风格：

- `feat: ...`
- `fix: ...`
- `docs: ...`
- `refactor: ...`
- `chore: ...`

建议继续保持这种格式。必要时可以添加简短的英文 scope：

```text
fix(qspi): correct fifo status check
docs(branching): clarify 2.1-dev release path
```

## 发布流程

发布 `v2.0.1` 这类补丁版本时，可以参考：

```bash
git fetch origin
git switch 2.0
git pull --ff-only origin 2.0

# 在这里执行项目验证。

git tag -a v2.0.1 -m "Release v2.0.1"
git push origin v2.0.1

git switch main
git pull --ff-only origin main
git merge --ff-only 2.0
git push origin main
```

如果 `main` 不能 fast-forward 合并，先停止，检查历史差异后再决定怎么处理。

发布 `v2.1.0` 这类小版本时，可以参考：

```bash
git fetch origin
git switch 2.1-dev
git pull --ff-only origin 2.1-dev

# 在这里执行项目验证。

git switch -c 2.1
git push origin 2.1
git tag -a v2.1.0 -m "Release v2.1.0"
git push origin v2.1.0

git switch main
git pull --ff-only origin main
git merge --ff-only 2.1
git push origin main
```

## 发布前最低验证

打发布标签前，至少要用一个已知模板工程验证主要支持板卡。

推荐的最低验证内容：

- 使用 `ecos init_project smoke_test` 创建一个全新的外部工程。
- 执行 Kconfig 默认配置。
- 执行 `make` 构建。
- 如果改动涉及 BSP、HAL、链接脚本或启动代码，需要对主要支持板卡重复验证。

不要直接在 `templates/` 目录里修改代码并把它当作测试工程。

## 第一次接手时建议补齐的管理项

1. 在 GitHub 上保护 `main`、`2.0`、`2.1-dev` 和 `1.0`。
2. 受保护分支要求通过 Pull Request 合并。
3. 合并前至少需要一名维护者 review。
4. 添加一个简单的 CI workflow，用来构建一个或多个模板工程。
5. 添加 `CHANGELOG.md`，每次发布时更新。
6. 添加单一版本来源，例如 `VERSION` 文件，让使用者不用查 Git tag 也能看到版本。
7. 决定 GitHub 默认展示分支应该是 `2.0` 还是 `main`。

## 简单判断规则

不确定一个改动应该放在哪个分支时，可以按下面的规则判断：

- 当前稳定版本的 bug 修复：放到 `2.0`
- 新功能或较大重构：放到 `2.1-dev`
- 旧版本兼容修复：放到 `1.0`
- 产品专属行为：放到 `starryskypi`
- 公开发布入口：放到 `main`
