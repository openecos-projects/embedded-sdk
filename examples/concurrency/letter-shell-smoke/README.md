# Letter-Shell 命令解析冒烟示例

例程名：`letter-shell-smoke`。本例初始化一个 1024 字节缓冲区的 Shell，
通过 `shellRun(&shell, "help")` 执行一次帮助命令，随后移除 Shell 并从 `main` 返回。
**它不是持续读取串口的交互式 Shell。**

## 板卡与准备

当前例程清单兼容 C1、C2、L3、L3.1、L4C1/C2/C3 和 T1-Pico。
3.0 CI 固件矩阵选用 L4C1、T1-Pico、C2；兼容不等于已经完成实机验收。
只需板卡供电、下载器和 UART 控制台，无外部存储要求。

## 创建、构建和运行

```bash
ecos project create letter-shell-smoke --board starrysky-l4-c1
cd letter-shell-smoke
ecos build
ecos flash
ecos monitor
```

其他 CI 板卡可将参数改为 `starrysky-t1-pico` 或 `starrysky-c2`。
必要时用 `ecos monitor --port /dev/ttyUSB0` 指定串口，波特率以所选 Board 配置为准。
使用 CI 固件时，选择 `letter-shell-smoke-<board-id>.bin` 并按对应板卡流程烧录。

## 测试步骤与预期结果

1. 先打开串口终端，再复位，避免错过一次性输出。
2. 确认 Shell 初始化及 `help` 输出正常；具体命令列表取决于构建配置。
3. 确认随后出现：

   ```text
   command parser completed
   ```

4. 无 panic、乱码或意外复位；断电重启后可重复得到上述结果。

`main` 返回后的行为由运行时决定，不要求本例继续提供提示符或响应键盘输入。
本例不验证回车、退格、方向键、历史记录、长输入或持续交互稳定性；
这些功能需在真正的交互式 Shell 应用中单独测试。

记录日期、硬件版本、Board ID、固件提交号/BIN SHA-256、完整输出及通过/失败结论。
