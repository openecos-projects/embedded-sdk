# SDK 自动发布

仓库通过 GitHub Actions 在推送正式版本标签时构建并发布 SDK 制品。触发标签必须使用
`v<major>.<minor>.<patch>` 形式，例如 `v3.0.0`；预发布标签可以使用
`v3.1.0-rc.1`。

## 发布条件

标签指向的提交必须同时满足：

- `tools/sdk-manifest.json` 中的 `sdk_version` 与去掉 `v` 的标签完全一致。
- 清单中的 `channel` 已从 `development` 改为 `release`。
- 仓库测试全部通过。
- 标签确实指向本次 workflow 检出的提交。
- 两个 SDK 归档会由 GitHub Artifact Attestations 生成 provenance 签名。

任一条件不满足时，workflow 会失败且不会创建 GitHub Release。

workflow 使用仓库内置的 `GITHUB_TOKEN`，不需要额外配置 PAT；仓库或组织策略必须允许
workflow 获得 `contents: write` 权限。

## 发布步骤

在版本分支完成测试后，先提交正式版本清单，再创建 annotated tag：

```bash
# tools/sdk-manifest.json:
#   "sdk_version": "3.0.0",
#   "channel": "release"

git add tools/sdk-manifest.json
git commit -m "chore(release): prepare v3.0.0"
git tag -a v3.0.0 -m "Release v3.0.0"
git push origin HEAD
git push origin v3.0.0
```

推送标签后，`.github/workflows/release.yml` 会依次执行测试、构建制品、验证归档中的
安装器、生成 Artifact Attestation，并创建 GitHub Release。发布会上传：

- `ecos-embedded-sdk-<version>.tar.gz`
- `ecos-embedded-sdk-<version>.zip`
- `SHA256SUMS`

Artifact Attestation 不作为普通 Release 文件上传，而是关联到 GitHub 仓库的 Attestations
记录中。它使用 GitHub Actions 的 OIDC 身份和短期 Sigstore 证书完成签名，不需要配置
GPG 私钥或额外的签名 Secret。当前仓库为公开仓库时可以直接使用；私有仓库需要 GitHub
Enterprise Cloud，GitHub Enterprise Server 不支持该功能。

预发布版本会自动标记为 GitHub prerelease。workflow 重跑时会先比较既有
`SHA256SUMS`，仅当制品内容完全一致时覆盖同名资产；如果同一版本的内容发生变化，发布
会失败，避免强制移动 tag 后替换已经交付的正式制品。

## 本地验证

发布脚本只打包 Git 中已经提交的正式 SDK 文件，不包含 `.git`、CI 配置、旧版 `bin/`
命令或工作区中的未提交文件。创建标签后可在本地运行：

```bash
python3 tools/release.py --tag v3.0.0 --output-dir dist
sha256sum -c dist/SHA256SUMS
```

联网时可用 GitHub CLI 验证归档的来源和签名：

```bash
gh attestation verify \
  ecos-embedded-sdk-3.0.0.tar.gz \
  -R openecos-projects/embedded-sdk

gh attestation verify \
  ecos-embedded-sdk-3.0.0.zip \
  -R openecos-projects/embedded-sdk
```

解压任一制品后，从归档根目录运行 `python3 tools/install.py` 即可安装 SDK。安装器仍会
根据宿主平台下载清单锁定的工具链和构建依赖，因此发布制品本身不重复捆绑多平台工具链。
