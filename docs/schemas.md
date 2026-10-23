# ECOS 3.0 资源清单 schema

Board、Target、Component、Example 和 SDK 清单使用随 Python CLI 离线分发的 JSON Schema。
原始契约分别是 [Board v2](../tools/ecos_cli/src/ecos_cli/resources/schemas/board.schema.json)、
[Target v1](../tools/ecos_cli/src/ecos_cli/resources/schemas/target.schema.json)、
[Component v1](../tools/ecos_cli/src/ecos_cli/resources/schemas/component.schema.json) 和
[Example v1](../tools/ecos_cli/src/ecos_cli/resources/schemas/example.schema.json)，以及
[SDK v1](../tools/ecos_cli/src/ecos_cli/resources/schemas/sdk.schema.json)。

```bash
ecos schema list --format json
ecos schema describe board --format json
ecos schema validate board boards/StarrySkyL3_1/ecos-board.yml --format json
```

`schema validate` 只检查单个 YAML/JSON 文件的结构、字段类型和取值范围；
`ecos validate --project PATH` 还检查文件是否存在、Board 到 Target 的映射、依赖、
能力和构建输入。四种清单的 `schema` 数字是清单格式版本，CLI 输出 envelope 的
`schema_version` 是机器接口版本，两者含义不同。未知字段会被拒绝；Board 中自定义
逻辑资源可以扩展，但必须是带 `driver` 的映射。

## Board v2

最小结构合法样例：

```yaml
schema: 2
id: demo-board
target: demo-soc
```

| 字段 | 含义 |
| --- | --- |
| `schema`, `id`, `target` | 必填；分别是格式版本、稳定 Board ID、Target ID。 |
| `name`, `aliases`, `category`, `arch` | 展示名、别名、产品分类和可选架构声明。 |
| `build.default_profile`, `build.profiles`, `build.components` | 默认 profile、profile 的 Kconfig 预设、板级构建单元 ID。 |
| `resources` | 逻辑资源映射；`gpio-demo`、`led`、`qspi-bus`、`soft-spi-bus`、`display` 等有专门字段约束。 |
| `kconfig.board` | Board 使用的板级 Kconfig 路径。 |
| `flash` | 显式烧录方式、固件类型及参数；`provider: mass-storage` 使用 `volume_label` 等字段，`provider: wlink` 使用 `chip`、`tool`、`unprotect`、`erase`。不声明时不自动推测。 |
| `monitor` | 串口参数（`baudrate`、可选的 `vid`/`pid`/`serial_number`）。 |
| `debug` | 可选 `cppdbg` 适配器、已运行调试服务器的 `server_address` 和入口暂停设置。 |

`flash.artifact` 选择 `artifacts.json` 中的产物，默认 `bin`。`wlink` provider 的
`chip` 是必填的芯片族（例如 `CH32V103`），`tool` 默认 `wlink`，`unprotect` 和
`erase` 分别默认 `false` 和 `true`；`unprotect` 会全片擦除受保护的芯片。

典型错误：删除 `target` 后执行 `ecos schema validate board PATH --format json`，
返回 `ECOS_SCHEMA_INVALID`，`field` 为 `/`，错误信息指出缺少 `target`。

`soft-spi-bus` 资源要求 `driver: soft-spi`、`chip_select`、`clock`、`mosi`、`miso`
四个 `{controller, pin}` GPIO 对象（pin 为 0 到 31），以及 0 到 3 的 `mode`。
配置阶段会把它生成为 `ECOS_BOARD_SOFT_SPI_*` 宏；四个引脚重复时，工程解析阶段拒绝该 Board。

## Target v1

最小结构合法样例：

```yaml
schema: 1
id: demo-soc
arch: riscv
cpu:
  march: rv32imac
  abi: ilp32
```

| 字段 | 含义 |
| --- | --- |
| `schema`, `id`, `arch`, `cpu.march`, `cpu.abi` | 必填；Target 身份和编译器 ISA/ABI。 |
| `name`, `description` | 展示名和说明。 |
| `toolchain` | 可选 `id`、`release`、`prefix`、`triple`，必须与 SDK 锁定的工具链一致。 |
| `memory` | 内存区域的 `origin`、`size` 和可选 `attributes`；大小可用 K/M 后缀。 |
| `capabilities` | Target 提供的硬件能力 ID。 |
| `debug` | Target 默认调试服务器连接；Board 的 `debug` 可覆盖它。 |
| `build` | CMake、toolchain、linker、Kconfig、源码、include、能力源码和输出类型。 |

完整工程校验还要求 Target 目录下存在 `CMakeLists.txt` 和 `toolchain.cmake`；
上面的最小样例只说明 schema 结构。典型错误：删除 `cpu.abi`，校验返回
`ECOS_SCHEMA_INVALID`，`field` 为 `/cpu`。

## Component v1

最小结构合法样例：

```yaml
schema: 1
id: driver-demo
```

| 字段 | 含义 |
| --- | --- |
| `schema`, `id` | 必填；格式版本和稳定构建单元 ID。 |
| `name`, `cmake_target` | 展示名和公开 CMake target，例如 `ecos::driver::demo`。 |
| `sources`, `include_dirs`, `defines` | 源码、公开头文件目录和编译宏。 |
| `dependencies`, `private_dependencies` | 向使用方传播的依赖和仅实现内部使用的依赖。 |
| `requires` | Target 或 Board 必须提供的能力；不是链接依赖。 |

典型错误：把 `cmake_target` 写成 `demo-driver`，校验返回 `ECOS_SCHEMA_INVALID`，
`field` 为 `/cmake_target`。依赖环、重复 ID 和公开 CMake target 冲突由工程解析器检查。

## Example v1

最小结构合法样例：

```yaml
schema: 1
name: hello
sources:
  - main.c
```

| 字段 | 含义 |
| --- | --- |
| `schema`, `name`, `sources` | 必填；格式版本、稳定 Example 名和非空源码列表。 |
| `include_dirs`, `defines` | 应用头文件目录和编译宏。 |
| `requires` | 运行所需逻辑资源或 Target 能力，例如 `console`、`display`。 |
| `components` | 应用需要的构建单元 ID。 |
| `unsupported_boards` | 显式排除的 Board ID。 |

典型错误：`sources: main.c` 不是列表，校验返回 `ECOS_SCHEMA_INVALID`，
`field` 为 `/sources`，`actual` 是 `main.c`，`expected` 是 `array`。

## SDK v1

最小结构合法样例：

```json
{
  "schema_version": 1,
  "sdk_id": "ecos-embedded-sdk",
  "sdk_version": "3.0.0",
  "channel": "development",
  "cli_compatibility": {"major": 3, "schema_version": "1.0"},
  "layout": {
    "boards": "boards", "components": "components", "drivers": "drivers",
    "devices": "devices", "docs": "docs", "examples": "examples",
    "hal": "hal", "cli": "tools/ecos_cli/src"
  },
  "toolchain": {"id": "xpack-riscv-none-elf-gcc", "release": "15.2.0-1"}
}
```

| 字段 | 含义 |
| --- | --- |
| `schema_version`, `sdk_id`, `sdk_version`, `channel` | 必填；清单格式、SDK 身份、版本和开发/发布通道。 |
| `cli_compatibility` | 必填；可使用的 CLI 主版本和机器接口 schema 版本。 |
| `layout` | 必填；八类 SDK 资源根路径。 |
| `toolchain` | 必填；默认锁定的工具链 ID 和 Release。 |
| `build.core_components` | 可选；每个工程默认加入的构建单元 ID。 |
| `build.kconfig` | 可选；3.0 工程共享的构建 Kconfig 文件，相对 SDK 根目录；声明后必须存在且进入发布包。 |
| `release.paths`, `release.exclude` | 发布通道必填的部署文件范围和可选排除路径。 |

`ecos schema validate sdk tools/sdk-manifest.json --format json` 检查结构；
SDK 注册和加载时还检查 CLI 兼容性、相对路径安全性及发布范围。
典型错误：把 `cli_compatibility.major` 设成 `true` 会返回
`ECOS_SCHEMA_INVALID`，`field` 为 `/cli_compatibility/major`，期望值为 `integer`。
