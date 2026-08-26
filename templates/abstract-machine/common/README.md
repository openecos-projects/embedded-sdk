# AbstractMachine 基础工程

`main.c` 是当前工程的 AM 应用源码。板卡的架构、默认 core、启动代码、链接脚本和
HAL 均由 BSP 的 AbstractMachine 环境提供，用户工程不包含板卡适配。

直接执行以下命令构建和清理：

```sh
make
make clean
```

产物位于 `build/retrosoc_fw.{elf,bin,txt,hex}`。完整说明见 SDK 的
`docs/abstract-machine.md`。
