# WDG 看门狗驱动

头文件：`drivers/wdg/include/ecos/driver/wdg.h`。
例程：`examples/peripherals/wdg/reset`。当前只有 StarrySky L3.1 声明 WDG 能力。

```c
typedef struct {
    uint32_t prescaler;
    uint32_t compare;
    uint32_t control;
} ecos_wdg_config_t;

#define ECOS_WDG_CONFIG_DEFAULT { 100u, 500u, 0x5u }

ecos_err_t ecos_wdg_init(ecos_wdg_id_t id, const ecos_wdg_config_t *config);
ecos_err_t ecos_wdg_deinit(ecos_wdg_id_t id);
ecos_err_t ecos_wdg_feed(ecos_wdg_id_t id);
ecos_err_t ecos_wdg_get_status(ecos_wdg_id_t id, uint32_t *status);
```

`prescaler` 和 `compare` 以实际分频/比较数表示，硬件寄存器写入值为参数减一，
因此两者都必须非零。`control` 是 SoC 原始控制字，默认 `0x5` 沿用旧复位例程。
驱动在每次配置、喂狗和停用前写入旧工程的解锁密钥；喂狗按旧序列向 FEED 写入
`1`、再写入 `0`。复位例程会启动看门狗，运行时可能复位开发板。

CH32V103 的 IWDG 使用 LSI 时钟，`prescaler` 只接受硬件支持的分频值
`4/8/16/32/64/128/256`，`compare` 为 1 到 4096 的 12-bit reload 值。
CH32V103 在写入 `0xCCCC` 启用 IWDG 后不能由软件关闭，`ecos_wdg_deinit()` 返回
`ECOS_ERR_UNSUPPORTED`，必须通过复位恢复。运行 CH32V103 的 `wdg-reset` 示例前，
应准备串口、烧录和恢复流程；它是破坏性复位测试，不是普通无副作用的 smoke test。
