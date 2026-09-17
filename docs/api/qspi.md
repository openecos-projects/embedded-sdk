# QSPI/SPI 主机驱动

面向显示控制器的裸帧写，以及 SPI-Flash 风格的命令/地址/dummy 事务。

头文件：`drivers/qspi/include/ecos/driver/qspi.h`。
示例：`example/peripherals/spi_master/st7789`（配合 ST7789 设备驱动）。

## 类型与常量

```c
typedef uint8_t ecos_qspi_id_t;                /* QSPI 控制器实例号 */

#define ECOS_QSPI_DEFAULT ((ecos_qspi_id_t)0u)  /* 默认实例 0 */

typedef enum {
    ECOS_QSPI_CS_0 = 0,
    ECOS_QSPI_CS_1,
    ECOS_QSPI_CS_2,
    ECOS_QSPI_CS_3,
    ECOS_QSPI_CS_COUNT
} ecos_qspi_cs_t;                              /* 片选线编号 */

typedef struct {
    uint32_t clock_divider;   /* 时钟分频；0 表示 Target 默认 */
} ecos_qspi_config_t;

#define ECOS_QSPI_CONFIG_DEFAULT { 0u }
```

## 初始化

```c
int ecos_qspi_get_instance_count(void);
ecos_err_t ecos_qspi_init(ecos_qspi_id_t qspi, const ecos_qspi_config_t *config);
ecos_err_t ecos_qspi_deinit(ecos_qspi_id_t qspi);
```

`ecos_qspi_get_instance_count()` 返回当前 Target 的 QSPI 控制器数量。

## 帧写（每次调用 = 一次 CS 断言）

默认使用 CS0；`_cs` 后缀版本在末参显式指定片选。

```c
ecos_err_t ecos_qspi_write_8(ecos_qspi_id_t qspi, uint8_t data);
ecos_err_t ecos_qspi_write_16(ecos_qspi_id_t qspi, uint16_t data);
ecos_err_t ecos_qspi_write_32(ecos_qspi_id_t qspi, uint32_t data);
ecos_err_t ecos_qspi_write_32_repeat(ecos_qspi_id_t qspi, uint32_t data,
                                     uint32_t words);
ecos_err_t ecos_qspi_write_32x2(ecos_qspi_id_t qspi,
                                uint32_t data1, uint32_t data2);
ecos_err_t ecos_qspi_write_32x8(ecos_qspi_id_t qspi,
                                uint32_t data1, uint32_t data2,
                                uint32_t data3, uint32_t data4,
                                uint32_t data5, uint32_t data6,
                                uint32_t data7, uint32_t data8);
ecos_err_t ecos_qspi_write_32x16(ecos_qspi_id_t qspi,
                                 uint32_t data1, /* ... 共 16 个 uint32_t ... */
                                 uint32_t data16);
ecos_err_t ecos_qspi_write_32x32(ecos_qspi_id_t qspi,
                                 uint32_t data1, /* ... 共 32 个 uint32_t ... */
                                 uint32_t data32);
```

显式片选版本（同名加 `_cs`，末参为 `ecos_qspi_cs_t`）：

```c
ecos_err_t ecos_qspi_write_8_cs(ecos_qspi_id_t qspi, uint8_t data,
                                ecos_qspi_cs_t cs);
ecos_err_t ecos_qspi_write_16_cs(ecos_qspi_id_t qspi, uint16_t data,
                                 ecos_qspi_cs_t cs);
ecos_err_t ecos_qspi_write_32_cs(ecos_qspi_id_t qspi, uint32_t data,
                                 ecos_qspi_cs_t cs);
ecos_err_t ecos_qspi_write_32x2_cs(ecos_qspi_id_t qspi, uint32_t data1,
                                   uint32_t data2, ecos_qspi_cs_t cs);
ecos_qspi_write_32x8_cs(...)    /* 8 个数据字 + cs */
ecos_qspi_write_32x16_cs(...)   /* 16 个数据字 + cs */
ecos_qspi_write_32x32_cs(...)   /* 32 个数据字 + cs */
```

- `write_8/16/32` — 单次写入 1/2/4 字节，常用于显示控制器的命令与参数。
- `write_32_repeat` — 同一 32 位字重复写 `words` 次，用于整屏/矩形填充。
- `write_32xN` — 一次 CS 断言内顺序写 N 个 32 位字（x16/x32 为 16/32 个独立的
  `uint32_t` 实参），用于打包像素流的批量下发。

## 命令/地址事务（固定使用 CS0）

SPI-Flash 风格的三段式事务，供 sfud 等组件使用：

```c
ecos_err_t ecos_qspi_send_cmd(ecos_qspi_id_t qspi,
                              uint8_t command, uint8_t command_bits,
                              uint32_t address, uint8_t address_bits);
ecos_err_t ecos_qspi_write(ecos_qspi_id_t qspi,
                           uint8_t command, uint8_t command_bits,
                           uint32_t address, uint8_t address_bits,
                           const void *data, size_t size);
ecos_err_t ecos_qspi_read(ecos_qspi_id_t qspi,
                          uint8_t command, uint8_t command_bits,
                          uint32_t address, uint8_t address_bits,
                          uint8_t dummy_cycles, void *data, size_t size);
```

- `command_bits` / `address_bits` — 命令与地址的线宽/位数，置 0 表示该段不发送。
- `ecos_qspi_send_cmd()` — 只发命令（+ 可选地址）。
- `ecos_qspi_write()` — 命令 + 地址 + 数据写。
- `ecos_qspi_read()` — 命令 + 地址 + dummy 周期 + 数据读。

## 最小使用骨架

```c
#include "ecos/bsp/console.h"
#include "ecos/driver/qspi.h"
#include "ecos/log.h"

#define LOG_TAG "qspi"

int main(void)
{
    const ecos_qspi_config_t config = { .clock_divider = 3u };

    ECOS_PANIC_ON_ERROR(LOG_TAG, bsp_console_init(), "initialize console");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_qspi_init(ECOS_QSPI_DEFAULT, &config),
                        "initialize QSPI");

    /* 写显示控制器：单字节命令 + 整框重复像素 */
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_qspi_write_8(ECOS_QSPI_DEFAULT, 0x2Cu),
                        "memory write command");
    ECOS_PANIC_ON_ERROR(LOG_TAG,
                        ecos_qspi_write_32_repeat(ECOS_QSPI_DEFAULT,
                                                  0xF800F800u, 128u * 128u / 2u),
                        "fill red");
    for (;;)
        __asm__ volatile("nop");
}
```

## 注意事项

- **每次写调用都是一次独立的 CS 断言**：跨调用不会被合并成同一帧，协议要求
  同一事务内的字节必须放进一次调用（如 `write_32xN`）。
- `write_32x16`/`write_32x32` 形参很多，实际应用通常经由 ST7735/ST7789 设备
  驱动（见 [devices.md](devices.md)）间接使用，不建议手写。
- 命令/地址事务组固定走 CS0；多从机场合只能用帧写组的 `_cs` 版本。
- 帧写参数 `write_32_repeat` 的 `words` 是 32 位字个数，不是字节数。
