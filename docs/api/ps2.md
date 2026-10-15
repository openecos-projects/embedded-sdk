# PS2 驱动

头文件：`drivers/ps2/include/ecos/driver/ps2.h`。
例程：`examples/peripherals/ps2/keycodes`。当前只有 StarrySky L3.1 声明 PS2 能力。

```c
ecos_err_t ecos_ps2_init(ecos_ps2_id_t id);
ecos_err_t ecos_ps2_deinit(ecos_ps2_id_t id);
ecos_err_t ecos_ps2_read(ecos_ps2_id_t id, uint32_t *scan_code);
```

`ecos_ps2_init()` 将 L3.1 的 GPIO1[12:13] 切换到 PS2 复用功能并启用控制器。
`ecos_ps2_read()` 返回原始寄存器值；零表示当前没有键码，不做键盘布局或按键状态解码。
旧例程使用 `0x76` 作为 Escape 键码。调用者应轮询读取，并在结束时调用
`ecos_ps2_deinit()`。
