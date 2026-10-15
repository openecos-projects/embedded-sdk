# SDRAM 驱动

`ecos/driver/sdram.h` 提供板级 SDRAM 控制器初始化：

```c
ecos_err_t result = ecos_sdram_init();
```

初始化时序、刷新周期和模式寄存器访问属于 Target HAL 的实现细节。T1-Pico 启动时
已初始化 SDRAM，重复调用该接口不会重置正在使用的外部内存。
`sdram-init` Example 从外部内存池分配 4 KiB 并校验读写；真实容量、长时间稳定性和
断电启动仍需按人工验收清单验证。
