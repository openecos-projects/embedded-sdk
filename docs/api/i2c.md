# I2C 主机驱动

7 位地址 I2C 主机收发，支持重复起始（repeated START）的写-读组合事务。

头文件：`drivers/i2c/include/ecos/driver/i2c.h`。
示例：`example/peripherals/i2c/scan`。

## 类型与常量

```c
typedef uint8_t ecos_i2c_id_t;              /* I2C 控制器实例号 */

#define ECOS_I2C_DEFAULT ((ecos_i2c_id_t)0u) /* 默认实例 0 */

typedef struct {
    uint32_t clock_divider;                  /* 时钟分频 */
} ecos_i2c_config_t;

#define ECOS_I2C_CONFIG_DEFAULT { 100u }
```

## 函数

```c
int ecos_i2c_get_instance_count(void);
ecos_err_t ecos_i2c_init(ecos_i2c_id_t i2c, const ecos_i2c_config_t *config);
ecos_err_t ecos_i2c_deinit(ecos_i2c_id_t i2c);
int ecos_i2c_probe(ecos_i2c_id_t i2c, uint8_t address);
ecos_err_t ecos_i2c_write(ecos_i2c_id_t i2c, uint8_t address,
                          const void *data, size_t size);
ecos_err_t ecos_i2c_read(ecos_i2c_id_t i2c, uint8_t address,
                         void *data, size_t size);
ecos_err_t ecos_i2c_write_read(ecos_i2c_id_t i2c, uint8_t address,
                               const void *write_data, size_t write_size,
                               void *read_data, size_t read_size);
```

- `ecos_i2c_get_instance_count()` — 返回当前 Target 提供的 I2C 控制器数量。
- `ecos_i2c_init()` / `ecos_i2c_deinit()` — 初始化/反初始化控制器。
- `ecos_i2c_probe()` — 探测地址：ACK 返回 1，NACK 返回 0，失败返回负错误码。
  用于总线扫描。
- `ecos_i2c_write()` / `ecos_i2c_read()` — 对 7 位从机地址发起完整写/读事务。
- `ecos_i2c_write_read()` — 先写若干字节，发出 repeated START，再读若干字节；
  读寄存器类设备的标准姿势。

## 最小使用骨架

```c
#include "ecos/bsp/console.h"
#include "ecos/driver/i2c.h"
#include "ecos/log.h"

#define LOG_TAG "i2c"

int main(void)
{
    const ecos_i2c_config_t config = ECOS_I2C_CONFIG_DEFAULT;
    int count;

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");

    count = ecos_i2c_get_instance_count();
    ECOS_PANIC_ON_ERROR(LOG_TAG, count, "query I2C controllers");
    if (count < 1)
        ECOS_PANIC_ON_ERROR(LOG_TAG, ECOS_ERR_NOT_FOUND, "find I2C controller");

    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_i2c_init(ECOS_I2C_DEFAULT, &config),
                        "initialize I2C");

    for (uint8_t addr = 0x08u; addr <= 0x77u; ++addr) {
        int ack = ecos_i2c_probe(ECOS_I2C_DEFAULT, addr);

        ECOS_PANIC_ON_ERROR(LOG_TAG, ack, "probe address");
        if (ack == 1)
            (void)ECOS_LOGI(LOG_TAG, "device at 0x%02X", addr);
    }

    ECOS_PANIC_ON_ERROR(LOG_TAG, ecos_i2c_deinit(ECOS_I2C_DEFAULT), "deinit");
    for (;;)
        __asm__ volatile("nop");
}
```

## 注意事项

- 地址一律为 **7 位**（不含读写位），范围 0x08–0x77 之外的保留地址不要扫。
- `probe` 的 0（NACK）是正常结果不是错误；只有负数才表示总线故障。
- 挂接设备驱动（AHT20、EEPROM 等）前必须先 `ecos_i2c_init` 同一实例，
  见 [devices.md](devices.md)。
- 不要与 2.x 遗留头文件 `hal/i2c/hal_i2c.h` 混用：其 `hal_i2c_config_t` 与新契约
  `ecos/hal/i2c.h` 中的同名类型冲突（见 [README.md](README.md) 的遗留警告）。
