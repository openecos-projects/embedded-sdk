# @BSP_NAME@ 外部 BSP 编写说明

本目录由 `ecos board create` 生成。它只是编写起点，不能直接作为可用 BSP。

## 必须完成

请按以下顺序处理：

1. `ecos-board.yml`：确认名称、处理器架构、默认 profile 以及所有文件路径。
2. `board.h`：根据芯片手册填写寄存器地址、位定义和板卡引脚。
3. `sections.lds`：根据真实 Flash、RAM 和栈空间填写内存布局。
4. `start.S`：完成复位入口、栈设置和进入 `main` 前的初始化。
5. `Makefile`：确认工具链、处理器指令集、ABI 和固件输出格式。
6. `build_conf.mk`：确认驱动源文件和 SDK 公共头文件的加入方式。
7. `board.kconfig`：填写内存大小、CPU 频率等板卡信息。
8. `driver.kconfig`：只启用已经实现并验证过的 HAL 驱动。
9. `driver/`：按照 SDK 的 HAL 头文件实现板卡驱动。
10. `templates/hello/`：确保最小示例只调用本 BSP 已实现的 HAL。

所有必须填写的位置都有 `TODO_BSP_REQUIRED`。完成后删除对应标记，然后执行：

```bash
ecos board check .
```

## 按需选择

- 需要生成分离式工程时，创建 BSP 时添加 `--with-isolated`，并完成 `Makefile_isolated`。
- 芯片需要额外加载阶段时，添加 `--with-loader`，并完成 `loader/loader.c`。
- 新增 I2C、QSPI、PWM 等外设时，在 `driver/`、`driver.kconfig` 和 `build_conf.mk` 中同时增加对应内容。
- 板卡需要专用示例时放在 `templates/<示例名>/`；不支持的示例不要提供空目录。
- 同一板卡有 Flash XIP、SRAM 等启动方式时，在 `profiles` 下增加项目；每项必须指定链接脚本和启动文件。

删除可选文件时，也要删除 `ecos-board.yml` 中对应的声明。

## 不要修改

- 2.x 外部 BSP 兼容脚手架中不要修改 `schema: 1`；SDK 3.0 内置 Board 使用独立的
  schema 2 资源定义。
- 不要修改 SDK HAL 头文件规定的函数名、参数和返回值。
- 不要修改 `BOARD_PACKAGE`、`BOARD_BUILD_CONF`、`BOARD_DRIVER_DIR` 等构建变量名。
- `start.S` 和 `sections.lds` 共同使用的符号必须保持一致；如确需改名，应同时修改两边。
- 不要把作者电脑上的绝对路径写入 Makefile 或清单。

## 完整流程

```bash
# 检查当前 BSP
ecos board check .

# 已经导入过同 id BSP 时，也可明确指定按更新包检查
ecos board check . --update @BSP_ID@

# 开发阶段：链接当前目录；驱动和 build_conf.mk 修改会直接用于下一次构建
ecos board add .

# board.h、启动文件、链接脚本或 Makefile 修改后，在已有普通工程内重新同步
ecos set_board @BSP_ID@

# 或正式导入：复制到 SDK/board/UserBSP/@BSP_ID@
ecos board import .

# 生成普通工程
ecos init_project hello -name hello_@BSP_ALIAS@ -target @BSP_ID@

# 选择指定的启动和内存布局
ecos init_project hello -name hello_@BSP_ALIAS@ -target @BSP_ID@ --profile flash_xip

# 生成分离式工程（BSP 必须提供 Makefile_isolated）
ecos init_project hello -isolated -name hello_@BSP_ALIAS@_isolated -target @BSP_ID@
```

可以参考处理器和启动方式最接近的内置 BSP，但寄存器地址、内存地址、时钟和引脚必须以目标芯片手册为准。
