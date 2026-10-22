# FatFs 已有卷挂载示例

例程名：`flash-fatfs`。本例调用 `load_filesystem()` 挂载已有的 `0:` 卷，
成功后输出就绪日志并从 `main` 返回。**它不自动格式化，也不创建或读写测试文件。**

## 板卡与数据安全

当前例程清单兼容 C1、C2、L3.1、L4C1/C2/C3 和 T1-Pico；
3.0 CI 选择其中 L4C1、T1-Pico、C2。兼容及构建通过不等于 Flash 和文件系统已经实机验证。
需要正确连接并配置 SFUD 后端使用的串行 Flash（当前移植为硬件 QSPI 端口 0），
且介质上已有可识别的 FAT 卷。核对板卡原理图、器件容量以及与 XIP/其他外设的共享约束。

**先备份现有数据。** 无文件系统时本例会报错退出，不会擦除介质。
`format_filesystem()` 是单独的破坏性操作，不是本例默认流程；
只有确认测试介质、容量、地址范围及数据备份后，才能在专门程序中显式调用。
禁止对存放固件或重要数据的区域盲目格式化。

## 创建、构建和运行

```bash
ecos project create flash-fatfs --board starrysky-c2
cd flash-fatfs
ecos build
ecos flash
ecos monitor
```

其他 CI 板卡可改为 `starrysky-l4-c1` 或 `starrysky-t1-pico`。
串口波特率以 Board 配置为准，可用 `ecos monitor --port /dev/ttyUSB0` 指定串口。
CI 固件名为 `flash-fatfs-<board-id>.bin`，按板卡下载流程烧录。
先打开串口终端再复位，以捕获一次性日志。

## 测试步骤与预期结果

1. 使用已知可用、已备份且已有 FAT 卷的介质，烧录并复位。
2. 应依次看到：

   ```text
   File system mounted
   FatFs volume is ready for file operations
   ```

3. 断电重启后再次确认可以挂载，无 panic 或异常复位。
4. 若介质未格式化或挂载失败，可能看到：

   ```text
   File system mount failed: <错误码>
   No existing filesystem; formatting was not requested
   ```

   后一条是通用失败提示，不足以证明介质确实为空；还需按错误码排查硬件和文件系统。
   对“已有卷挂载”验收来说，这属于失败，不能当成自动格式化成功。

本例只验证挂载，不证明文件创建、写入、关闭后重读、擦除块几何或掉电数据保持正确。
完整存储验收需在专用测试卷上另外完成上述操作。
记录硬件版本、Flash 型号/容量、卷准备方式、固件提交号/BIN SHA-256、日志和结论。
