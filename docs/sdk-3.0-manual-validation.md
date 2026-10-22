# SDK 3.0 人工验收清单

自动化回归已经覆盖清单解析、依赖/能力拒绝、工程生成、CMake/Ninja 构建、产物契约、
模拟烧录/串口、release 路径校验和 VS Code 客户端单元测试。下面项目需要真实宿主、
板卡或外部设备，完成后应在对应复选框后补充日期、硬件版本和固件提交号。

## 跨平台宿主

- [ ] GNU/Linux x86_64：从 release archive 安装 xPack `15.2.0-1`，完成 `sdk register/list/use/pin`、工程创建、配置、构建、卸载。
- [ ] GNU/Linux arm64：完成同一安装和最小 `hello` 构建，确认工具链归档和 SHA-256 与清单一致。
- [ ] macOS x86_64 和 arm64：分别完成安装、工程路径含空格和非 ASCII 字符的配置/构建。
- [ ] Windows x64：在没有 Bash、GNU Coreutils、GNU Make 和 `minicom` 的环境完成安装、配置、构建、`.cmd` CLI 启动、烧录和串口监视。
- [ ] 两个 SDK 版本并存：验证 active SDK、项目 pin、单次 `--sdk` 的优先级以及同版本摘要冲突拒绝。
- [ ] release 安装失败恢复：模拟校验失败、磁盘空间/权限错误和中断，确认 staging 不会留下半成品或覆盖已有版本。

## 板卡与外设

- [ ] C1：hello、GPIO、ST7735、I2C 设备、mass-storage flash 和 UART monitor。
- [ ] C2：flash-xip 与 psram-data 启动、PSRAM 数据读写、ST7735/ST7789、PWM 和共享 QSPI。
- [ ] T1-Pico：LED/Button、I2C/QSPI、CLINT、PLIC 分配检查、GPIO 边沿计数、Timer 周期计数、SDRAM 4 KiB 读写及长时间稳定性、真实模型文件的 `llm-chat` 启动和串口输出。
- [ ] L3/L3.1：flash-xip 与 mem loader 启动、L3.1 PS2/WDG/RTC/RNG/CRC/RCU，以及 UART monitor。
- [ ] L4 C1/C2/C3：hello、GPIO、ST7735/ST7789 画面、按钮/LED、电源和 QSPI 刷新稳定性。
- [ ] 每块板确认 Board 清单中的串口波特率、mass-storage 卷标、LED 有效电平和显示接线与实物一致。
- [ ] 对至少一块板执行真实 `ecos flash` 后断电重启，确认固件确实从目标启动，而不是只完成文件复制。
- [ ] 在带外置 SPI Flash 的板卡上运行 `flash-sfud`，确认 JEDEC/容量识别、跨页写入、擦除后读取校验和掉电重启结果。
- [ ] 在同一块带外置 SPI Flash 的板卡上运行 `flash-fatfs`，确认按 SFUD 容量计算的扇区数和擦除块大小；对已存在卷完成挂载、创建/写入/关闭/重新打开/读取校验及掉电重启。空 Flash 只在备份后显式调用 `format_filesystem()`，确认格式化结果后再重复读写。
- [ ] 在声明了 `soft-spi-bus` 资源的 Board 上运行 `soft-spi-jedec-id` 集成程序（C2 当前提供构建示例），确认示例只使用 `ecos/board_resources.h` 的引脚；再按外设手册分别将资源 mode 设为 0、1、2、3 重新构建，验证单字节收发、片选边界、JEDEC 识别和连续传输稳定性。
- [ ] 在 C2 或 L4-C1 上运行 `letter-shell-smoke`，确认启动提示符、`help`、命令执行、回车/退格/方向键、历史记录和长输入边界；持续输入 30 分钟，确认无串口阻塞或内存破坏。
- [ ] 在已挂载 FatFs 卷的 C2 或 L4-C1 上运行 `text-editor` 示例，先执行 `ls`、`cd`、`touch`，确认提示符目录同步、列目录结果和 `touch` 不截断已有文件；再执行 `edit /notes.txt` 与相对路径编辑，验证方向键、回车、退格、Tab、`Ctrl+W` 保存、`Esc` 退出与未保存内容的二次确认。退出后重新打开文件并断电重启再次读取，核对内容；同时确认 80x24 终端的状态栏、长文件名及满缓冲提示。C1/C2 还需确认串口无输入时不会产生伪字符，独立 `Esc` 不会阻塞。
- [ ] 在 C2 已有 FatFs 卷上运行 `fcc` 示例，分别用 `flash-xip` 和 `psram-data` 固件编译并执行 [示例源码](../examples/storage/fcc/README.md)：确认 `fcc-run` 返回 42、全局变量初值与前向函数调用正确；再运行循环程序、非法源码和超大源码，确认正常返回或明确拒绝，不会破坏 Shell。确认 `cd` 后相对路径能打开文件，断电后卷仍可挂载。
- [ ] 对至少一块板执行 `ecos monitor --expect ...`，确认超时、断开和重新连接诊断可操作。

## VS Code 与调试

- [ ] Windows、GNU/Linux、macOS 桌面扩展宿主完成工程识别、Board/profile 切换、configure/build/clean Task。
- [ ] 未信任工作区只能查询；确认不会写入 `.ecos`、下载工具链、构建、烧录或打开串口。
- [ ] 多根工作区分别执行两个工程任务，确认 cwd、输出通道和 Problems 不串项目。
- [ ] 真实调试能力 Target：启动 GDB server，确认扩展使用锁定工具链 GDB、ELF 和声明的 server address。
- [ ] 取消 configure/build/monitor 任务，确认子进程退出且不会留下错误的“成功”状态。

## 发布与供应链

- [ ] 从 tag 生成 tar.gz/zip，确认包含清单声明的 3.0 路径及全部公共组件/示例，且不包含 `UserBSP` 和被排除的内部测试目录；对 FatFs、Letter-Shell、letter-shell-fs、text-editor、fcc 完成对应实板验收并记录日期、硬件版本和提交号。
- [ ] 核对 SPDX SBOM 列出的 Python wheel、CMake/Ninja、xPack 和归档文件的许可证，将审查结论及 SBOM 随正式发布归档。
- [ ] 核对 CI 五类 runner 上传的安装与最小构建报告，以及 Python 3.9 最低版本报告；Linux x64 报告还需包含 L4C1（`starrysky-l4-c1`）、T1-Pico（`starrysky-t1-pico`）、C2（`starrysky-c2`）各自默认 profile 下的全部兼容 Example/ISA/ABI 组合、BIN 大小上限及 ELF/BIN 摘要，并检查 `ecos-firmware-matrix-<commit>` 中的命名、清单和 SHA256 校验和。
