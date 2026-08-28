# ysyx-2512 SoC Target

`ysyx-2512` 是星空 L4 板卡使用的 Target/SoC。本目录统一保存原先位于
`board/StarrySkyL4/` 中的芯片级资料：

- `include/`：内存映射寄存器定义。
- `startup/`：复位入口和 PSRAM 加载程序。
- `linker/`：Flash 和 PSRAM 链接脚本。
- `hal/`：公共外设 HAL API 的 SoC 实现。
- `Kconfig`：SoC 外设能力配置项。
- `build.mk`：RV32E/ILP32E 固件构建和镜像生成规则。

Board 清单负责描述 Board ID、板级资源和默认 profile，并将 `starrysky-l4`
映射到本 Target。Board 不再提供另一份芯片实现。

在 SDK 源码目录下，可按以下方式创建并构建最小 Example：

```bash
export ECOS_SDK_HOME=/path/to/embedded-sdk
python3 "$ECOS_SDK_HOME/tools/ecos.py" --sdk "$ECOS_SDK_HOME" \
    project create hello --board starrysky-l4
cd hello
make
```

构建完成后会生成 `build/retrosoc_fw.elf`、`.bin`、`.txt` 和 `.hex`。
