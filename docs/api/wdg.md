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
