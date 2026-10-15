# 从 SDK 2.x 迁移工程到 3.0

3.0 工程使用 Python `ecos` CLI、清单、CMake 和 Ninja。旧工程的 `Makefile`、
`build_conf.mk`、板卡启动文件和链接脚本是迁移输入，不复制到新工程。先确认目标
Board 已支持所需能力；历史板卡和 Example 的现状见
[迁移状态](sdk-3.0-legacy-board-migration.md)。

## 迁移应用

1. 在 2.x 工程中列出应用源码、使用的外设、板载资源、Kconfig 选项和启动 profile。
   将旧 `main.c` 中直接读写 `REG_*`、引用 `board.h`、选择 GPIO 引脚或 PinMux 的代码
   标记出来，分别归入 Target HAL、公共 Driver 或 Board BSP。
2. 查询 3.0 可用的 Board、Example、Driver 和 Device Driver：

   ```bash
   ecos board describe starrysky-l4-c1 --format json
   ecos project list --format json
   ecos driver list --format json
   ecos device list --format json
   ```

3. 从最小 `hello` Example 建立工程，并编辑工程内的应用源码和
   `ecos-example.yml`。`--path` 是已有的父目录；工程会创建在其下：

   ```bash
   ecos project create hello --name my-app --path /path/to/workspace \
     --board starrysky-l4-c1 --dry-run --format json
   ecos project create hello --name my-app --path /path/to/workspace \
     --board starrysky-l4-c1
   ecos validate --project /path/to/workspace/my-app --format json
   ecos build --project /path/to/workspace/my-app
   ```

4. 在工程的 `ecos-example.yml` 中明确列出应用源码、`components` 和 `requires`。
   `components` 是稳定构建单元 ID；`requires` 是 Board 资源或 Target 能力，不能
   用头文件目录或 Make 变量代替。例如 GPIO 演示应用要求 `gpio-demo`，并通过生成的
   `ecos/board_resources.h` 获取引脚。控制台、LED 和按键使用 `ecos/bsp/*`；总线
   使用 `ecos/driver/*`；外部芯片使用 `ecos/device/*`。具体调用见
   [公共 API](api/README.md)。
5. 将旧 Kconfig 值通过 `ecos menuconfig --project /path/to/workspace/my-app`
   重新设置；Board 只给默认值，工程配置保存在 `.ecos/project.config`，不要编辑
   `.ecos/generated/`。使用 `ecos project set-profile` 选择 Board 声明的启动布局。

旧 `ecos init_project` 对应 `ecos project create`。切换 Board 使用
`ecos project set-board BOARD --project PATH`；该命令同时选择 Board 映射的 Target，
不会替换应用源码。仅选择 SoC 时使用 `ecos project set-target TARGET --project PATH`，
这会清除 Board；需要 LED、显示等板载资源的应用应选择 Board。切换后运行
`ecos validate --project PATH` 和 `ecos build --project PATH`，缺少的能力会在编译前报出。

## 迁移 BSP

内置 Board 可参考 `boards/StarrySkyL4_C1/ecos-board.yml` 和同目录的
`ecos-component.yml`。用户 Board 放在安装文档声明的用户 BSP 目录，开发时可用
`ECOS_USER_BSP_ROOT` 指向该目录。迁移时按以下顺序拆分：

1. 选择现有 `components/soc/<target>/` 中的 Target。只有芯片 CPU/ISA/ABI、启动、
   寄存器或外设实现确实不同，才需要新建 Target。不要把某块板的引脚和外设接线写进
   Target；也不要在 Board 组件里复制 Target 的启动文件或链接规则。
2. 在 `ecos-board.yml` 写入稳定 Board ID、`target`、`build.components`、可选
   profile、`resources`、`flash` 和 `monitor`。`console`、`led`、`button`、
   `gpio-demo`、显示和总线接线应由 Board 清单表达；能否使用外设由 Target capability
   与 Board resource 共同决定。旧 `board.h` 和 `build_conf.mk` 不能作为 3.0 构建来源。
3. 用 `ecos-component.yml` 声明 BSP 源码、公开 include、稳定 ID 和依赖。BSP 对
   Driver 的依赖一般为 `private_dependencies`；应用只包含 BSP 公开头文件。
4. 校验清单和工程解析，再用 `hello`、`blink`、`display-donut` 中实际兼容的示例
   验证 Console、GPIO 和设备链路：

   ```bash
   ecos schema validate board /path/to/UserBSP/MyBoard/ecos-board.yml --format json
   ecos schema validate component /path/to/UserBSP/MyBoard/ecos-component.yml --format json
   ecos board describe my-board --format json
   ecos project list --format json
   ```

`schema validate` 只查单个文件结构；工程的依赖、能力、源码和工具链组合还需
`ecos validate`。对硬件语义的验证结果应记录在
[人工验收清单](sdk-3.0-manual-validation.md)。

SFUD、LightCoroutine、FatFs、Letter-Shell、letter-shell-fs、soft-spi、text-editor 和 fcc 已加入 3.0 开发清单图：`sfud` 复用现有 QSPI HAL，
`light-coroutine` 提供任务创建、上下文保存和协作式调度，`fatfs` 通过 SFUD 按设备元数据
提供 FatFs 磁盘适配，`letter-shell` 通过 3.0 BSP console 提供命令解析和串口端口层，
`letter-shell-fs` 使用 FatFs 提供 `cd`、`ls`、非破坏性 `touch` 命令，`text-editor` 通过
BSP console 和 FatFs 提供 `edit` 命令，`soft-spi` 通过 GPIO 引脚配置提供 mode 0 到 mode 3
的 bit-bang SPI；`soft-spi-jedec-id` 使用 Board 的 `soft-spi-bus` 资源读取外置 Flash JEDEC ID。
示例 `flash-sfud` 只验证初始化和 Flash 识别链路，
`flash-fatfs` 只验证构建和非破坏性挂载调用；两者的擦写、文件读写和掉电恢复仍需按人工验收
清单在实板完成。`coroutine-smoke` 在无外设依赖的有限轮次中验证协程运行时。
`letter-shell-smoke` 只验证命令表初始化和有限命令解析；`text-editor` 示例可在已有卷上运行
`edit <filename>`，需要 Target 具备至少 64 KiB RAM，且尚未完成实板读写验收。FatFs、
Letter-Shell、letter-shell-fs、text-editor 和 fcc 当前仍从正式 release 排除。soft-spi 已进入
3.0 组件构建图并提供 C2 的绑定引脚示例；其他 Board 仍需在清单中声明自己的 `soft-spi-bus`
资源。fcc 目前只支持
C2：组件要求 `rv32im-exec-sram`，链接脚本为生成代码预留片内 SRAM，并对代码、跳转和
全局数据做容量检查。两个 C2 profile 已通过构建；实际执行和文件系统操作需按人工清单验收。
依赖未迁移组件的应用应先完成对应组件及其总线、堆或文件系统适配，再在应用清单中引用稳定
组件 ID；当前源码树中保留的旧头文件和模板不能直接当作 3.0 公共接口使用。
