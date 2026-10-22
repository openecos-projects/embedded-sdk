# SFUD 串行 Flash 识别示例

例程名：`flash-sfud`。调用 `sfud_init()`，检查设备 0 是否初始化成功，
输出 Flash 名称和容量后从 `main` 返回。**本例不执行擦除、编程或数据读取校验。**

## 板卡、接线与安全

当前例程清单兼容 C1、C2、L3.1、L4C1/C2/C3 和 T1-Pico；
3.0 CI 选择其中 L4C1、T1-Pico、C2。清单兼容及构建通过不证明实物已经装有可用 Flash。
当前 SFUD 移植使用硬件 QSPI 端口 0，接线、片选、供电和 Flash 型号应核对对应板卡原理图，
不能随意把器件连接到其他 SPI 引脚。

测试前备份重要数据，确认 QSPI 控制器与固件 XIP/其他外设的共享约束。
本例虽不主动擦写用户数据，初始化也不能视为对任意共享总线均无影响。
不要为了运行此识别例程而擦除已有文件系统。

## 创建、构建和运行

```bash
ecos project create flash-sfud --board starrysky-c2
cd flash-sfud
ecos build
ecos flash
ecos monitor
```

其他 CI 板卡可改为 `starrysky-l4-c1` 或 `starrysky-t1-pico`。
串口波特率以 Board 配置为准，必要时使用 `ecos monitor --port /dev/ttyUSB0`。
CI 固件名为 `flash-sfud-<board-id>.bin`，按对应板卡流程烧录。
先打开串口终端再复位，避免漏掉一次性日志。

## 测试步骤与预期结果

1. 复位，检查 SFUD 初始化日志中识别出的器件信息。
2. 应出现 `Flash <名称> ready, capacity <字节数> bytes`。
3. 将名称及容量与实物型号、数据手册核对，容量应合理且非零。
4. 断电重启后重复检查，识别应一致。

`SFUD initialization failed: ...`、`No initialized serial flash device`、
器件或容量不符均判为失败，保留完整日志并检查接线、供电、片选和总线共享。
程序结束后不持续打印是正常现象。

验收结论只覆盖初始化及识别，不代表跨页写入、擦除、全容量读写或掉电数据保持通过。
这些操作需另设测试程序，在确认并备份的专用区域进行，严禁直接覆盖固件或文件系统。
记录硬件版本、Flash 型号、固件提交号/BIN SHA-256、实际识别信息和测试结论。
