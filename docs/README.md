# ECOS Embedded SDK 3.x 文档索引

面向应用的公共文档：

- [install.md](install.md) — 安装指南：Python 安装器、安装路径、工具链处理与多版本切换
- [cli.md](cli.md) — `ecos` CLI 命令、JSON 输出和工程工作流
- [versioning.md](versioning.md) — `v3.1` 开发身份、维护边界、SemVer 与后续 RC / 正式发布门槛
- [agent-skill.md](agent-skill.md) — ECOS SDK Agent Skill：工程/适配/排错/验收流程及显式安装
- [schemas.md](schemas.md) — Board、Target、Component 和 Example 的离线 schema 与样例
- [VS Code 扩展](../tools/vscode-ecos/README.md) — 3.x CLI 的编辑器预览集成与构建任务
- [sdk-3.0-manual-validation.md](sdk-3.0-manual-validation.md) — 3.0 历史基线的宿主、板卡和外部设备验收清单
- [ch32v103-windows-test-checklist.md](ch32v103-windows-test-checklist.md) — CH32V103 第一阶段（原 `3.0.1`）Windows 实机历史记录；分支已撤销，复现方式见 [versioning.md](versioning.md)
- [toolchain-cli.md](toolchain-cli.md) — Python 工具链命令：`ecos` CLI 的工具链识别、状态检查与安装
- [api/README.md](api/README.md) — 3.x 公共 API 概览：分层架构、错误模型、配置惯例与应用骨架
- [api/bsp.md](api/bsp.md) — BSP：控制台、LED、按键与板级资源宏
- [api/core.md](api/core.md) — 核心运行时：错误码与日志
- [api/gpio.md](api/gpio.md) — GPIO 驱动
- [api/uart.md](api/uart.md) — UART 驱动
- [api/i2c.md](api/i2c.md) — I2C 主机驱动
- [api/pwm.md](api/pwm.md) — PWM 驱动
- [api/timer.md](api/timer.md) — Timer 驱动
- [api/qspi.md](api/qspi.md) — QSPI/SPI 主机驱动
- [api/archinfo.md](api/archinfo.md) — Archinfo 芯片标识驱动
- [api/rng.md](api/rng.md) — RNG 随机数驱动
- [api/crc.md](api/crc.md) — CRC 校验驱动
- [api/rcu.md](api/rcu.md) — RCU 时钟复位驱动
- [api/rtc.md](api/rtc.md) — RTC 计数器驱动
- [api/ps2.md](api/ps2.md) — PS2 键盘原始键码驱动
- [api/wdg.md](api/wdg.md) — WDG 看门狗驱动
- [api/clint.md](api/clint.md) / [api/interrupt.md](api/interrupt.md) — CLINT 和机器中断驱动
- [api/gpio-interrupt.md](api/gpio-interrupt.md) / [api/timer-interrupt.md](api/timer-interrupt.md) — GPIO/Timer 中断驱动
- [api/sdram.md](api/sdram.md) — SDRAM 初始化驱动
- [api/devices.md](api/devices.md) — 设备驱动：传感器、EEPROM、蜂鸣器、WiFi 模组、RTC、显示屏、数码管
- [api/components.md](api/components.md) — libc、heap、LightCoroutine、sfud、FatFs、Letter-Shell、letter-shell-fs、soft-spi、text-editor 与 fcc 使用说明
