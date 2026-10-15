# CLINT 驱动

`ecos/driver/clint.h` 提供 Target 的机器计时器读取和比较值访问。当前迁移实现覆盖
StarrySky T1-Pico 的 CL1-2512，`mtime` 频率通过 `ecos_clint_get_frequency_hz()` 查询。

```c
ecos_clint_value_t now;
ecos_clint_get_mtime(&now);
```

该接口只返回计数值，不把 CLINT 地址或寄存器布局暴露给应用。`mtimecmp` 的写入由
Target HAL 负责按高低字安全更新。
