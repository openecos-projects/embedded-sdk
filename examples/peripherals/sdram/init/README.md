# SDRAM 初始化与 4 KiB 读写示例

例程名：`sdram-init`。当前兼容板卡：StarrySky T1-Pico（`starrysky-t1-pico`）。
需使用实际装有 SDRAM 的板卡，并连接供电、下载器及 UART 控制台；无需额外接线。

## 创建、构建和运行

```bash
ecos project create sdram-init --board starrysky-t1-pico
cd sdram-init
ecos build
ecos flash
ecos monitor
```

T1-Pico 控制台默认 115200 波特率。可用 `ecos monitor --port /dev/ttyUSB0` 指定串口。
先打开监视终端，再复位，避免漏掉一次性日志。
CI 对应固件为 `sdram-init-starrysky-t1-pico.bin`，按板卡下载流程烧录。

## 测试步骤与预期结果

1. 复位板卡。程序初始化 SDRAM，从外部堆分配 1024 个 `uint32_t`（4096 字节）。
2. 程序逐字写入 `0xA5A50000 ^ index`，再逐字读取比较，最后释放内存。
3. 应看到：

   ```text
   SDRAM 4 KiB read/write check passed
   ```

4. 输出后进入空循环，不再打印是正常现象。断电重启后重复验证。

任何初始化失败、`external memory allocation failed` 或 `SDRAM mismatch at word ...`
均判为失败，保留完整串口日志。此测试会覆盖所分配的测试内存，不写入 Flash。

本例只做一次 4 KiB 检查，不代表全容量、全部地址线、长时间稳定性或高负载验证通过。
记录硬件版本、固件提交号/BIN SHA-256、供电条件、重启次数、日志和测试结论。
