# 外部 BSP 创建、导入与项目生成教程

本文介绍如何为 ECOS SDK 编写一个外部 BSP，并用它生成普通工程或分离式工程。

## 1. 完整流程

```text
ecos board create
        |
        v
阅读 README 和文件内中文注释
        |
        v
填写芯片和板卡信息
        |
        v
ecos board check
        |
        v
ecos board import
        |
        v
ecos init_project
```

正式导入的 BSP 默认复制到：

```text
<ECOS SDK>/board/UserBSP/<BSP id>/
```

这样不会把用户 BSP 混入 SDK 自带的板卡目录。

重新运行 SDK 的 `python3 tools/install.py` 时会保留目标 SDK 中已有的
`board/UserBSP/` 内容。

### SDK 环境

首次使用，或 SDK 目录移动后，可在当前终端执行：

```bash
eval "$(ecos env)"
```

该命令设置 `ECOS_SDK_HOME`，并把 SDK 命令和工具链目录加入 `PATH`。普通工程生成时还会把当时的 SDK 位置写入工程内的 `configs/sdk-path.mk`，所以关闭终端后通常也能直接运行 `make defconfig`。分离式工程自带配置工具，不读取这个文件。

如果 SDK 后来被移动，普通工程可以重新执行上面的命令，也可以修改工程内的 `configs/sdk-path.mk`。普通工程发现 SDK 路径为空或无效时，Makefile 会直接停止并提示这两个修复方法；分离式工程不受 SDK 位置变化影响。

## 2. 生成基本 BSP 目录

在准备存放 BSP 的目录中执行：

```bash
ecos board create acme-demo-v1 \
    -name "ACME Demo V1" \
    -output ./acme-demo-v1-bsp
```

可选参数：

| 参数 | 作用 |
| --- | --- |
| `-name <名称>` | 设置显示名称 |
| `-output <目录>` | 设置输出目录，默认是 `./<BSP id>-bsp` |
| `--with-isolated` | 同时生成分离式工程 Makefile |
| `--with-loader` | 同时生成 loader 示例 |

例如，需要分离式工程和额外加载代码时：

```bash
ecos board create acme-demo-v1 \
    -name "ACME Demo V1" \
    --with-isolated \
    --with-loader
```

命令不会覆盖已经存在的目录。

## 3. 生成后的目录

基础目录如下：

```text
acme-demo-v1-bsp/
├── README.md
├── ecos-board.yml
├── Makefile
├── board.h
├── board.kconfig
├── driver.kconfig
├── build_conf.mk
├── sections.lds
├── start.S
├── driver/
│   ├── sys_uart/
│   │   └── sys_uart.c
│   └── gpio/
│       └── gpio.c
└── templates/
    └── hello/
        ├── main.c
        ├── main.h
        └── configs/
            └── defconfig
```

使用可选参数后还会生成 `Makefile_isolated` 或 `loader/loader.c`。

每个生成文件都有中文说明，并使用下面三种标记：

```text
[必须修改] 必须根据目标芯片或板卡填写。
[按需修改] 只在使用对应功能时修改。
[不要修改] SDK 命令或公共接口依赖该名称和结构。
```

`TODO_BSP_REQUIRED` 表示尚未完成的必填项。完成对应代码或配置后，需要删除该标记。

## 4. 必须填写的内容

### 4.1 `ecos-board.yml`

这个文件告诉 SDK BSP 中各文件的位置。

- 2.x 外部 BSP 兼容流程生成的 `schema` 保持为 `1`。SDK 3.0 内置 Board 已开始迁移到
  schema 2；外部 BSP 的 schema 2 导入要等 Python 资源解析器完成后开放。
- `id` 必须以小写字母开头，只能包含小写字母、数字和短横线；导入后不要随意改变。
- `arch` 根据实际处理器确认。
- 文件改名或移动后，同步修改 `files`、`kconfig` 或 `paths` 中的相对路径。
- profile 名称必须以小写字母开头，只能包含小写字母、数字和下划线，即符合 `^[a-z][a-z0-9_]*$`。
- `default_profile` 指定不传 `--profile` 时使用的布局；它必须是 `profiles` 中已有的名称。
- `default_profile` 可以不写，此时工具会选择 `profiles` 中声明的第一项。为避免调整顺序后默认布局变化，建议明确填写。
- `profiles` 中每一项都要指定链接脚本和启动文件。
- BSP 提供 AbstractMachine 环境时，在 `abstract_machine.path` 中声明板内环境目录，
  并用 `abstract_machine.default_core` 指定默认核心 profile。
- 不允许使用绝对路径或 `../` 指向 BSP 目录外。

新生成的清单默认包含：

```yaml
default_profile: flash_xip
profiles:
  flash_xip:
    linker_script: sections.lds
    startup: start.S
```

只有一套布局时保留这一项即可。旧 BSP 使用 `files.linker_script` 和 `files.startup` 的写法仍然可以导入和生成工程。

可选的 AbstractMachine 声明示例：

```yaml
abstract_machine:
  path: environments/abstract-machine
  default_core: rv32e-base
```

默认 core 必须存在于 `<path>/cores/<default_core>.conf`。应用开发者创建工程时直接
采用 BSP 默认值，不从命令行选择 core。所有 AM 应用和 am-kernels 模板只依赖 AM
接口；板卡相关的启动、链接、架构参数和设备实现必须全部封装在该 BSP 环境中。
环境目录至少要提供 `board.mk`、`build.mk` 和默认 core 配置文件；`build.mk` 负责
把通用 AM 构建变量绑定到 BSP 的平台实现。

### 4.2 `board.h`

根据芯片手册填写：

- 外设寄存器地址。
- 寄存器位定义。
- 板卡引脚和外设实例对应关系。
- 驱动需要的其他板级常量。

生成文件中的 `0x00000000` 等数值只是无效占位值，不能直接使用。

### 4.3 `sections.lds`

根据芯片内存表填写：

- Flash 起始地址和长度。
- RAM 或 PSRAM 起始地址和长度。
- 代码、只读数据、已初始化数据和未初始化数据的位置。
- 栈位置和大小。

链接脚本中的符号必须与 `start.S` 使用的符号一致。

### 4.4 `start.S`

根据芯片启动方式完成：

- 设置栈。
- 必要的上电初始化。
- 搬运 `.data`。
- 清零 `.bss`。
- 最终调用 `main`。

如果芯片已有厂家启动代码，可以在保持入口和链接符号一致的前提下进行适配。

### 4.5 `Makefile` 和 `build_conf.mk`

需要确认：

- 交叉编译工具链前缀。
- 处理器指令集和 ABI。
- 链接入口和链接脚本。
- 最终需要生成的 ELF、BIN、HEX 或其他固件格式。
- 每个已启用驱动对应的源文件和 HAL 头文件路径。

不要把本机绝对路径写入构建文件。普通工程中的外部 BSP 路径使用 `BOARD_PACKAGE` 或 `BOARD_DRIVER_DIR`，SDK 路径使用 `ECOS_SDK_HOME`。分离式工程的 Makefile 只能引用工程内的 `Library/`、`Startup/`、`System/`、`Hardware/`、`User/`、`scripts/` 和 `tools/` 等目录。

### 4.6 HAL 驱动

外部 BSP 实现 SDK `hal/` 目录中已有的接口。例如 GPIO 驱动包含：

```c
#include "hal_gpio.h"
#include "board.h"
```

函数名、参数和返回值必须与 SDK 头文件一致。寄存器操作留在 BSP 驱动内部，应用代码只调用 HAL。

没有实现的驱动不要在 `driver.kconfig` 中默认打开，也不要让示例工程调用它。

## 5. 按需选择的内容

### 分离式工程

需要生成可脱离 SDK 原目录的工程时，在创建 BSP 时使用 `--with-isolated`，并完成 `Makefile_isolated` 中的必填项。

生成时会复制 BSP、HAL、组件源码、Kconfig、fixdep 和构建脚本，但不会复制 SDK 中已经编译的 Kconfig/fixdep 二进制或 `.d` 依赖文件。工程生成完成后，执行 `make defconfig`、`make menuconfig` 和 `make` 都不再读取 `ECOS_SDK_HOME`。

第一次运行配置命令时，工程会在当前电脑从源码构建本地工具，因此需要主机 `gcc`、`g++`、`make`、`flex` 和 `bison`；使用 `make menuconfig` 还需要 ncurses 开发库。固件构建仍需要 BSP Makefile 指定的交叉编译工具链。

### Loader

只有芯片需要从 Flash 搬运代码、二级启动或其他加载过程时，才使用 `--with-loader`。同时需要在 `start.S` 中正确调用加载代码。

### 更多驱动

添加 I2C、QSPI、PWM 等驱动时，需要同时处理：

1. 在 `driver/<驱动名>/` 中实现 HAL。
2. 在 `driver.kconfig` 中增加开关。
3. 在 `build_conf.mk` 中加入源文件和 HAL 头文件目录。
4. 增加能验证该驱动的板卡专用模板。

## 6. 检查 BSP

完成必填项后，在 BSP 目录外执行：

```bash
ecos board check ./acme-demo-v1-bsp
```

也可以在 BSP 目录内执行：

```bash
ecos board check .
```

检查内容包括：

- 必需清单字段是否存在。
- ID 和别名格式是否正确、是否重复。
- 清单声明的文件和目录是否存在。
- 路径是否越过 BSP 根目录。
- 是否还存在 `TODO_BSP_REQUIRED`。

检查只验证目录和文件关系，不能代替编译和硬件验证。

### 检查已导入 BSP 的更新

同一个 BSP 已经导入后，可以继续在原始源码目录执行普通检查。工具会自动识别相同的 `id`，将它作为待更新版本，并显示与 `board/UserBSP/` 中副本的目录差异。

自动识别不方便时，也可以明确指定目标：

```bash
ecos board check ./acme-demo-v1-bsp --update acme-demo-v1
```

这只做检查和差异显示，不会覆盖已导入 BSP。若源 BSP 的 `id` 与目标不同，或别名被另一块板占用，命令仍会按真正的冲突报错。

## 7. 导入 BSP

检查通过后执行：

```bash
ecos board import ./acme-demo-v1-bsp
```

BSP 会被复制到：

```text
board/UserBSP/acme-demo-v1/
```

如果同名 BSP 已存在，默认不会覆盖。确认需要替换时执行：

```bash
ecos board import ./acme-demo-v1-bsp --replace
```

新 BSP 会先检查和复制完成，再替换旧目录。

建议替换前先单独检查：

```bash
ecos board check ./acme-demo-v1-bsp --update acme-demo-v1
ecos board import ./acme-demo-v1-bsp --replace
```

## 8. BSP 开发模式

开发过程中可以不复制 BSP，而是链接原目录：

```bash
ecos board add ./acme-demo-v1-bsp
```

该方式适合开发，不适合发布固定版本，但不同文件的生效方式不同：

- `driver/` 中的驱动和 `build_conf.mk` 由普通工程通过 `BOARD_PACKAGE` 引用，保存后会反映到下一次构建。
- `board.h`、启动文件、链接脚本和 BSP Makefile 会被复制到工程中。修改 BSP 原目录后，需要进入已有工程重新执行 `ecos set_board acme-demo-v1`；使用了非默认 profile 时还要带上 `--profile <名称>`。
- `templates/` 只在创建工程时复制。模板修改只影响之后生成的工程，不会覆盖已有工程中的 `main` 等用户文件。

分离式工程始终使用复制后的文件。修改 BSP 源目录后，需要在 SDK 环境中进入已有工程并执行 `ecos set_board_isolated acme-demo-v1`，或者重新生成分离式工程；使用了非默认 profile 时同样要带上 `--profile <名称>`。

查看已发现的 BSP：

```bash
ecos board list
ecos board info acme-demo-v1
```

删除导入副本或开发链接：

```bash
ecos board remove acme-demo-v1
```

该命令不会删除最初传入的 BSP 源码目录，也不会删除已经生成的工程。

## 9. 生成普通工程

```bash
ecos init_project hello \
    -name hello_acme \
    -target acme-demo-v1

cd hello_acme
make defconfig
make
```

如果 BSP 有多套布局，可在生成时选择：

```bash
ecos init_project hello \
    -name hello_acme_sram \
    -target acme-demo-v1 \
    --profile sram
```

未传 `--profile` 时使用清单中的 `default_profile`。生成工程会把选中的链接脚本复制为 `sections.lds`，把启动文件复制为 `start.S`，并在 `.ecos-project` 中记录本次选择。

模板查找顺序是：

1. SDK 中明确支持该板卡的模板。
2. 外部 BSP 的 `templates/<模板名>/`。
3. SDK 的通用模板。

找不到合适模板时命令会停止，不会自动使用 C2 或其他板卡模板。

## 10. 生成分离式工程

BSP 已提供并完成 `Makefile_isolated` 时执行：

```bash
ecos init_project hello \
    -isolated \
    -name hello_acme_isolated \
    -target acme-demo-v1
```

板卡文件会复制到：

```text
Library/BoardDrivers/   BSP HAL 驱动
Library/BoardConfig/    BSP 配置文件
Library/board.h         板卡定义
Startup/                启动文件、链接脚本和可选 loader
User/                   示例 main 文件
scripts/                本地构建脚本
tools/kconfig/          本地配置工具和配置入口
tools/fixdep/           本地依赖处理工具
```

`tools/kconfig/build/` 和 `tools/fixdep/build/` 不会从 SDK 复制，首次配置时会在当前电脑重新生成。

生成完成后，分离式工程不再依赖 BSP 原目录或 SDK 原目录，也不会生成 `configs/sdk-path.mk`。即使 `ECOS_SDK_HOME` 未设置或指向无效目录，本地配置和构建仍使用工程内的 `scripts/` 与 `tools/`。交叉编译工具链以及必要的主机编译环境仍需要由用户准备。

## 11. 常见问题

### `check` 报告 `TODO_BSP_REQUIRED`

对应位置仍是占位代码或未确认配置。按中文注释完成后删除该标记，再重新检查。

### 导入时提示 ID 或别名已被使用

使用 `ecos board list` 查找冲突。不要让不同 BSP 共用同一个 ID 或别名。

如果是在修改已经导入的同一个 BSP，请使用 `ecos board check <源码目录> --update <id>`；这类情况会显示为更新检查，而不是普通冲突。

### `make defconfig` 访问 `/tools/kconfig`

新生成的普通工程会保存 SDK 路径，不应再出现该路径。旧的普通工程可以执行 `eval "$(ecos env)"` 后重新构建，或重新生成工程的 `configs/sdk-path.mk`。新生成的分离式工程使用本地 `tools/kconfig`，不会访问 `/tools/kconfig`。

### 找不到工程模板

在 BSP 中增加 `templates/<模板名>/`，或者选择已经提供的模板。不要复制不兼容板卡的模板来绕过检查。

### 普通工程移动后找不到 BSP

普通工程仍依赖 SDK 和已导入 BSP。需要脱离 SDK 原目录时使用 `-isolated`；分离式工程会携带 BSP、HAL、组件源码和配置工具，但不会携带交叉编译工具链。
