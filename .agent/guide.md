# 🤖 AI Agent Guide for ECOS Embedded SDK

本文档是专门面向 AI 编程助手（Agent）的操作指南和系统约束。当你在处理与本 SDK 相关的任务时，**必须**严格遵循以下规则。

## 🚫 核心红线 (CRITICAL RULES)

1. **绝对禁止**在 SDK 本体目录（如 `templates/`）下直接进行测试编译。
2. **所有编译测试**必须在 `testdir/` 的子目录下独立进行。
3. **禁止交互式命令**：在配置 Kconfig 时，绝不要使用 `make menuconfig`，这会阻塞进程。使用无头的 `conf --alldefconfig` 等非交互命令生成配置。

## 🛠️ 模板迁移标准操作程序 (SOP)

当你被要求将旧模板（如适配 c1/c2 的工程）迁移到新板卡（如 `L3_1`）时，请执行以下标准流程：

### 步骤 1: 建立测试工程
- 必须在 `testdir/` 下创建对应的工程目录，例如 `testdir/<template_name>`。
- 复制目标源文件 (`main.c`, `main.h`, `sections.lds`, `start.s` 等) 到测试目录中进行修改。
- 注意：`testdir` 目录已配置 `direnv`，会自动加载 `BoardExport`、`DriverExport` 等环境变量。如果你通过脚本执行编译，请确保这些环境变量已生效。若需要在其他目录下使用，需要先执行安装脚本才能同步更新。

### 步骤 2: API 升级与适配 (Legacy API -> HAL V2)
在源码中进行以下重构，注意旧版直接写寄存器的方式必须被完全废弃：
- **串口通信**: `sys_uart_init()` 替换为 `hal_sys_uart_init()`
- **引脚复用与配置**: 废弃直接写寄存器的方式。使用 `gpio_hal_set_fcfg(port, pin, func)` 和 `gpio_hal_set_mux(port, pin, mux)`。
- **外设初始化**:
  - `pwm_init(...)` -> `pwm_hal_init(...)`
  - `qspi_init(...)` -> `hal_qspi_init(...)`
  - `i2c_init(...)` -> `hal_i2c_init(...)`
- **定时器与系统滴答**: 移除对 `REG_TIM0_CONFIG` 等底层寄存器的操作。
  - `sys_tick_init()` 替换为 `hal_sys_tick_init(0)`
  - `get_sys_tick()` 替换为 `hal_get_sys_tick(0)`
  - 延时需求优先使用 `hal_delay_ms()`。

### 步骤 3: 自动化配置与编译
- 在 `testdir/<template_name>` 中执行无头配置生成（避免交互阻塞）：
  ```bash
  /tools/kconfig/build/conf --alldefconfig Kconfig
  /tools/kconfig/build/conf --syncconfig Kconfig
  ```
  *(注：如果存在直接 `make` 报错缺少 `.config` 或 `libgcc.h` 时，必须执行此步骤。)*
- 执行并行编译：`make -j$(nproc)`
- **强制检查**: 监控编译日志，你**必须**解决所有 `implicit declaration of function`（隐式函数声明）警告或错误，否则视为迁移失败。

### 步骤 4: 同步回 SDK
- **仅当**在 `testdir` 下编译完全通过后，将最终验证可用的源码文件（如 `main.c`）拷贝或覆盖到 SDK 模板对应的目标板卡专属目录中，路径格式为：`templates/<template_name>/<board_name>/` (如 `templates/spi_st7735/l3_1/main.c`)。
