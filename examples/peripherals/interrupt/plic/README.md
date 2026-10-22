# PLIC 中断源分配检查

例程名：`plic`。当前兼容板卡：StarrySky T1-Pico（`starrysky-t1-pico`）。
验证公共中断接口对中断源 3 的分配、重复分配拒绝、使能、禁用及释放。
无需外接中断信号，只需供电、下载器和 UART 控制台。

## 创建、构建和运行

```bash
ecos project create plic --board starrysky-t1-pico
cd plic
ecos build
ecos flash
ecos monitor
```

控制台默认 115200 波特率；可用 `ecos monitor --port /dev/ttyUSB0` 指定串口。
先打开终端再复位。CI 对应固件为 `plic-starrysky-t1-pico.bin`，按板卡流程烧录。

## 测试步骤与预期结果

1. 复位。程序初始化 PLIC，以优先级 1 分配中断源 3。
2. 第二次分配同一中断源应返回 `ECOS_ERR_BUSY`。
3. 程序设置阈值为 0，再使能、禁用并释放中断源。
4. 全部检查通过时输出：

   ```text
   PLIC source 3 allocation check passed
   ```

输出后进入空循环。出现 `duplicate PLIC allocation was accepted` 或其他 panic
均判为失败，保存日志。不要把本例与另一个占用中断源 3 的程序合并运行。

本例不触发真实外部中断，也不验证 ISR 分发、抢占或中断时延；这些需另外测试。
记录日期、硬件版本、固件提交号/BIN SHA-256、串口输出及通过/失败结论。
