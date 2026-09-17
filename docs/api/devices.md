# 设备驱动

设备驱动在外设驱动（[i2c](i2c.md)、[pwm](pwm.md)、[uart](uart.md)、
[qspi](qspi.md)、[gpio](gpio.md)）之上封装具体芯片：AHT20 温湿度计、AT24C64
EEPROM、PWM 蜂鸣器、ESP01S WiFi 模组、SGP30 空气质量计、PCF8563 RTC、
ST7735/ST7789 显示屏、TM1650 数码管。

## 通用模式

所有设备驱动遵循同一结构：

- **config 结构体** `ecos_<dev>_config_t`：指定挂接的总线实例与芯片参数
  （I2C 地址、PWM 通道、QSPI 片选、GPIO 引脚等）；
- **默认值宏** `ECOS_<DEV>_CONFIG_DEFAULT`：对常见板卡接线的默认初始化器；
- **句柄结构体** `ecos_<dev>_t`：内含 `config` 副本与 `initialized` 标志，
  由应用持有并传给每个 API；
- 所有函数返回 `ecos_err_t`（见 [core.md](core.md) 的错误模型）。

使用顺序固定为：**应用先初始化总线驱动**（如 `ecos_i2c_init`），**再调用
`ecos_<dev>_init`**：

```c
const ecos_i2c_config_t bus_config = ECOS_I2C_CONFIG_DEFAULT;
const ecos_aht20_config_t dev_config = ECOS_AHT20_CONFIG_DEFAULT;
ecos_aht20_t sensor;

ECOS_PANIC_ON_ERROR(TAG, ecos_i2c_init(ECOS_I2C_DEFAULT, &bus_config), "i2c");
ECOS_PANIC_ON_ERROR(TAG, ecos_aht20_init(&sensor, &dev_config), "aht20");
```

---

## AHT20（I2C 温湿度传感器）

头文件：`devices/aht20/include/ecos/device/aht20.h`。
示例：`example/peripherals/i2c/aht20`。

```c
#define ECOS_AHT20_I2C_ADDRESS 0x38u   /* 7 位 I2C 地址 */

typedef struct {
    ecos_i2c_id_t i2c;      /* 挂接的 I2C 控制器实例 */
    uint8_t address;        /* 设备地址 */
} ecos_aht20_config_t;

typedef struct {
    ecos_aht20_config_t config;
    uint8_t initialized;
} ecos_aht20_t;

typedef struct {
    int32_t temperature_x100;   /* 摄氏温度 ×100（定点，可为负） */
    uint32_t humidity_x100;     /* 相对湿度 % ×100（定点） */
} ecos_aht20_data_t;

#define ECOS_AHT20_CONFIG_DEFAULT { ECOS_I2C_DEFAULT, ECOS_AHT20_I2C_ADDRESS }

ecos_err_t ecos_aht20_init(ecos_aht20_t *sensor, const ecos_aht20_config_t *config);
ecos_err_t ecos_aht20_deinit(ecos_aht20_t *sensor);
ecos_err_t ecos_aht20_read(ecos_aht20_t *sensor, ecos_aht20_data_t *data);
```

- 读数为 ×100 定点数，避免在裸机环境引入浮点格式化；打印整数/小数部分请
  自行拆分（示例中有参考实现）。
- `ecos_aht20_read()` 触发一次测量并返回换算结果。

## AT24C64（I2C EEPROM）

头文件：`devices/at24c64/include/ecos/device/at24c64.h`。
示例：`example/peripherals/i2c/at24c64`。

```c
#define ECOS_AT24C64_I2C_ADDRESS 0x50u    /* A2/A1/A0 接地时的 7 位地址 */
#define ECOS_AT24C64_CAPACITY_BYTES 8192u /* 容量 8 KB */
#define ECOS_AT24C64_PAGE_BYTES 32u       /* 页大小 32 字节 */

typedef struct { ecos_i2c_id_t i2c; uint8_t address; } ecos_at24c64_config_t;
typedef struct { ecos_at24c64_config_t config; uint8_t initialized; } ecos_at24c64_t;

#define ECOS_AT24C64_CONFIG_DEFAULT { ECOS_I2C_DEFAULT, ECOS_AT24C64_I2C_ADDRESS }

ecos_err_t ecos_at24c64_init(ecos_at24c64_t *eeprom,
                             const ecos_at24c64_config_t *config);
ecos_err_t ecos_at24c64_deinit(ecos_at24c64_t *eeprom);
ecos_err_t ecos_at24c64_read(ecos_at24c64_t *eeprom, uint16_t memory_address,
                             void *data, size_t size);
ecos_err_t ecos_at24c64_write(ecos_at24c64_t *eeprom, uint16_t memory_address,
                              const void *data, size_t size);
```

- `init` 会 probe 设备地址。
- `ecos_at24c64_write()` 自动按 32 字节页边界拆分，并在每页之后等待内部写
  周期完成，应用无需关心分页。
- `memory_address` 范围 0..8191。

## 蜂鸣器 buzzer（PWM 输出）

头文件：`devices/buzzer/include/ecos/device/buzzer.h`。
示例：`example/peripherals/pwm/buzzer`。

```c
typedef struct {
    ecos_pwm_id_t pwm;            /* PWM 控制器实例 */
    ecos_pwm_channel_t channel;   /* 输出通道 */
    uint32_t pwm_clock_hz;        /* PWM 控制器输入时钟，用于换算音调 */
} ecos_buzzer_config_t;

typedef struct { ecos_buzzer_config_t config; uint8_t initialized; } ecos_buzzer_t;

/* StarrySky L4 板载蜂鸣器：PWM0 通道 0，SoC PWM 按 CPU 时钟计数 */
#define ECOS_BUZZER_CONFIG_DEFAULT { ECOS_PWM_DEFAULT, ECOS_PWM_CHANNEL_0, 50000000u }

ecos_err_t ecos_buzzer_init(ecos_buzzer_t *buzzer, const ecos_buzzer_config_t *config);
ecos_err_t ecos_buzzer_deinit(ecos_buzzer_t *buzzer);
ecos_err_t ecos_buzzer_play_tone(ecos_buzzer_t *buzzer, uint32_t frequency_hz);
ecos_err_t ecos_buzzer_beep(ecos_buzzer_t *buzzer, uint32_t frequency_hz,
                            uint32_t duration_ms);
ecos_err_t ecos_buzzer_stop(ecos_buzzer_t *buzzer);
```

- `ecos_buzzer_init()` 只保存配置，蜂鸣器保持静默直到发声。
- `ecos_buzzer_play_tone()` 以 50% 占空比持续发声，直到 `stop`。
- `ecos_buzzer_beep()` 阻塞发声 `duration_ms` 毫秒后自动停止。
- 蜂鸣器驱动自行初始化 PWM 控制器，应用不要再对同一 PWM 实例 `init`。

## ESP01S（UART AT WiFi 模组）

头文件：`devices/esp01s_at/include/ecos/device/esp01s_at.h`。
示例：`example/peripherals/uart/esp01s`。

```c
typedef enum {
    ECOS_ESP01S_MODE_STA = 1,     /* Station 模式 */
    ECOS_ESP01S_MODE_AP = 2,      /* AP 模式 */
    ECOS_ESP01S_MODE_AP_STA = 3   /* 混合模式 */
} ecos_esp01s_mode_t;

typedef enum {
    ECOS_ESP01S_CONNECTION_SINGLE = 0,  /* 单连接（CIPMUX=0） */
    ECOS_ESP01S_CONNECTION_MULTI = 1    /* 多连接（CIPMUX=1） */
} ecos_esp01s_connection_mode_t;

typedef struct {
    ecos_uart_port_t uart;   /* 串口实例 */
    uint32_t baud_rate;      /* 波特率 */
} ecos_esp01s_config_t;

typedef struct { ecos_esp01s_config_t config; uint8_t initialized; } ecos_esp01s_t;

/* ESP01S 在 StarrySky L4 板上接 UART1（hp 块） */
#define ECOS_ESP01S_CONFIG_DEFAULT { ECOS_UART_PORT_1, 115200u }

ecos_err_t ecos_esp01s_init(ecos_esp01s_t *module, const ecos_esp01s_config_t *config);
ecos_err_t ecos_esp01s_deinit(ecos_esp01s_t *module);
ecos_err_t ecos_esp01s_get_version(ecos_esp01s_t *module, char *version, size_t size);
ecos_err_t ecos_esp01s_set_mode(ecos_esp01s_t *module, ecos_esp01s_mode_t mode);
ecos_err_t ecos_esp01s_set_connection_mode(ecos_esp01s_t *module,
                                           ecos_esp01s_connection_mode_t mode);
ecos_err_t ecos_esp01s_wifi_join(ecos_esp01s_t *module, const char *ssid,
                                 const char *password);
ecos_err_t ecos_esp01s_tcp_connect(ecos_esp01s_t *module, const char *host,
                                   const char *port);
ecos_err_t ecos_esp01s_enter_passthrough(ecos_esp01s_t *module);
ecos_err_t ecos_esp01s_send(ecos_esp01s_t *module, const void *data, size_t size);
```

- `ecos_esp01s_init()` 完成 UART 初始化、AT 握手并关闭回显。
- `ecos_esp01s_get_version()` 发 `AT+GMR`，第一行应答以 NUL 结尾拷入
  `version`（最多 `size - 1` 字节）。
- `ecos_esp01s_wifi_join()` 加入 WPA/WPA2 热点，空中握手需要数秒。
- `ecos_esp01s_tcp_connect()` 要求先选单连接模式（CIPMUX=0）。
- `ecos_esp01s_enter_passthrough()` 进入透传（CIPMODE=1 + CIPSEND，等待
  `>`），之后用 `ecos_esp01s_send()` 写原始字节。

## SGP30（I2C 空气质量传感器）

头文件：`devices/gy_sgp30/include/ecos/device/sgp30.h`。
示例：`example/peripherals/i2c/sgp30`。

```c
#define ECOS_SGP30_I2C_ADDRESS 0x58u

typedef struct { ecos_i2c_id_t i2c; uint8_t address; } ecos_sgp30_config_t;
typedef struct { ecos_sgp30_config_t config; uint8_t initialized; } ecos_sgp30_t;

typedef struct {
    uint16_t co2_eq_ppm;   /* eCO2 浓度（ppm） */
    uint16_t tvoc_ppb;     /* TVOC 浓度（ppb） */
} ecos_sgp30_air_quality_t;

typedef struct {
    uint16_t co2_eq_baseline;   /* eCO2 基线 */
    uint16_t tvoc_baseline;     /* TVOC 基线 */
} ecos_sgp30_baseline_t;

#define ECOS_SGP30_CONFIG_DEFAULT { ECOS_I2C_DEFAULT, ECOS_SGP30_I2C_ADDRESS }

ecos_err_t ecos_sgp30_init(ecos_sgp30_t *sensor, const ecos_sgp30_config_t *config);
ecos_err_t ecos_sgp30_deinit(ecos_sgp30_t *sensor);
ecos_err_t ecos_sgp30_read_serial_id(ecos_sgp30_t *sensor, uint64_t *serial_id);
ecos_err_t ecos_sgp30_measure_air_quality(ecos_sgp30_t *sensor,
                                          ecos_sgp30_air_quality_t *air_quality);
ecos_err_t ecos_sgp30_get_baseline(ecos_sgp30_t *sensor,
                                   ecos_sgp30_baseline_t *baseline);
ecos_err_t ecos_sgp30_set_baseline(ecos_sgp30_t *sensor,
                                   const ecos_sgp30_baseline_t *baseline);
```

- `init` 会 probe 设备并启动 IAQ 测量；芯片启动后**前 15 秒返回默认值**
  （400 ppm / 0 ppb），属正常现象。
- 基线（baseline）可用于掉电保存/恢复，缩短再次上电的稳定时间。

## PCF8563（I2C RTC）

头文件：`devices/pcf8563/include/ecos/device/pcf8563.h`。
示例：`example/peripherals/i2c/pcf8563`。

```c
#define ECOS_PCF8563_I2C_ADDRESS 0x51u

typedef struct { ecos_i2c_id_t i2c; uint8_t address; } ecos_pcf8563_config_t;
typedef struct { ecos_pcf8563_config_t config; uint8_t initialized; } ecos_pcf8563_t;

typedef struct {
    uint8_t second;   /* 0-59 */
    uint8_t minute;   /* 0-59 */
    uint8_t hour;     /* 0-23 */
    uint8_t day;      /* 1-31 */
    uint8_t weekday;  /* 0-6 */
    uint8_t month;    /* 1-12 */
    uint8_t year;     /* 0-99，相对 2000 年的偏移 */
} ecos_pcf8563_time_t;

#define ECOS_PCF8563_CONFIG_DEFAULT { ECOS_I2C_DEFAULT, ECOS_PCF8563_I2C_ADDRESS }

ecos_err_t ecos_pcf8563_init(ecos_pcf8563_t *rtc, const ecos_pcf8563_config_t *config);
ecos_err_t ecos_pcf8563_deinit(ecos_pcf8563_t *rtc);
ecos_err_t ecos_pcf8563_get_time(ecos_pcf8563_t *rtc, ecos_pcf8563_time_t *time);
ecos_err_t ecos_pcf8563_set_time(ecos_pcf8563_t *rtc,
                                 const ecos_pcf8563_time_t *time);
```

- `init` 只 probe 设备，不触碰 RTC 寄存器（不会破坏正在走时的时钟）。
- 时间字段为十进制语义，驱动内部完成 BCD 转换；`year` 是相对 2000 的偏移。

## ST7735（QSPI TFT，128×128）

头文件：`devices/st7735/include/ecos/device/st7735.h`。
示例：`example/peripherals/spi_master/st7735`（配合 `ECOS_BOARD_DISPLAY_*` 宏）。

```c
typedef struct {
    ecos_qspi_id_t qspi;             /* QSPI 控制器实例 */
    ecos_qspi_cs_t chip_select;      /* 片选 */
    ecos_gpio_port_t dc_port;        /* D/C 引脚（命令/数据选择） */
    uint8_t dc_pin;
    ecos_gpio_port_t reset_port;     /* 复位引脚 */
    uint8_t reset_pin;
    ecos_gpio_port_t backlight_port; /* 背光引脚 */
    uint8_t backlight_pin;
    uint16_t width;                  /* 面板宽（像素） */
    uint16_t height;                 /* 面板高（像素） */
    uint8_t rotation;                /* 旋转 */
    uint8_t horizontal_offset;       /* 显存水平偏移 */
    uint8_t vertical_offset;         /* 显存垂直偏移 */
    uint32_t qspi_clock_divider;     /* QSPI 时钟分频 */
} ecos_st7735_config_t;

typedef struct { ecos_st7735_config_t config; uint8_t initialized; } ecos_st7735_t;

#define ECOS_ST7735_CONFIG_DEFAULT \
    { ECOS_QSPI_DEFAULT, ECOS_QSPI_CS_0, ECOS_GPIO_PORT_0, 29u, \
      ECOS_GPIO_PORT_0, 30u, ECOS_GPIO_PORT_0, 31u, \
      128u, 128u, 0u, 0u, 0u, 3u }

ecos_err_t ecos_st7735_init(ecos_st7735_t *display, const ecos_st7735_config_t *config);
ecos_err_t ecos_st7735_deinit(ecos_st7735_t *display);
ecos_err_t ecos_st7735_set_window(ecos_st7735_t *display, uint16_t x, uint16_t y,
                                  uint16_t width, uint16_t height);
ecos_err_t ecos_st7735_fill(ecos_st7735_t *display, uint16_t x, uint16_t y,
                            uint16_t width, uint16_t height, uint32_t color);
```

- `init` 期间 RST 与背光引脚各拉高一次。
- `ecos_st7735_fill()` 的 `color` 是**两个打包的 RGB565 像素**（高 16 位先发），
  例如红色 `0xF800F800u`；填充 `(w×h)/2` 个 32 位字。
- 移植时优先用 `ecos/board_resources.h` 的 `ECOS_BOARD_DISPLAY_*` 宏填充
  config（见 [bsp.md](bsp.md) 与 st7735 示例）。

## ST7789（QSPI TFT，240×240）

头文件：`devices/st7789/include/ecos/device/st7789.h`。
示例：`example/peripherals/spi_master/st7789`。

与 ST7735 完全同构（config 字段、函数签名一一对应），差别只在默认面板参数：

```c
#define ECOS_ST7789_CONFIG_DEFAULT \
    { ECOS_QSPI_DEFAULT, ECOS_QSPI_CS_0, ECOS_GPIO_PORT_0, 29u, \
      ECOS_GPIO_PORT_0, 30u, ECOS_GPIO_PORT_0, 31u, \
      240u, 240u, 0u, 0u, 0u, 3u }

ecos_err_t ecos_st7789_init(ecos_st7789_t *display, const ecos_st7789_config_t *config);
ecos_err_t ecos_st7789_deinit(ecos_st7789_t *display);
ecos_err_t ecos_st7789_set_window(ecos_st7789_t *display, uint16_t x, uint16_t y,
                                  uint16_t width, uint16_t height);
ecos_err_t ecos_st7789_fill(ecos_st7789_t *display, uint16_t x, uint16_t y,
                            uint16_t width, uint16_t height, uint32_t color);
```

`color` 同样是两个打包的 RGB565 像素，高 16 位先发。

## TM1650（GPIO 位 bang 四位数码管）

头文件：`devices/tm1650/include/ecos/device/tm1650.h`。
示例：`example/peripherals/gpio/tm1650`。

```c
#define ECOS_TM1650_POSITION_COUNT 4u     /* 4 位 */
#define ECOS_TM1650_SEGMENT_DOT 0x80u     /* 小数点位（bit7 = DP） */
#define ECOS_TM1650_BRIGHTNESS_OFF 0u     /* 关显示 */
#define ECOS_TM1650_BRIGHTNESS_MAX 8u
#define ECOS_TM1650_BRIGHTNESS_DEFAULT 3u

typedef struct {
    ecos_gpio_port_t dat_port;  /* 数据线引脚 */
    uint8_t dat_pin;
    ecos_gpio_port_t clk_port;  /* 时钟线引脚 */
    uint8_t clk_pin;
} ecos_tm1650_config_t;

typedef struct {
    ecos_tm1650_config_t config;
    uint8_t brightness;
    uint8_t initialized;
} ecos_tm1650_t;

/* StarrySky L4C1 板载接线：SEG_DAT = GPIO1[9]，SEG_CLK = GPIO1[10] */
#define ECOS_TM1650_CONFIG_DEFAULT { ECOS_GPIO_PORT_1, 9u, ECOS_GPIO_PORT_1, 10u }

ecos_err_t ecos_tm1650_init(ecos_tm1650_t *display, const ecos_tm1650_config_t *config);
ecos_err_t ecos_tm1650_deinit(ecos_tm1650_t *display);
ecos_err_t ecos_tm1650_set_brightness(ecos_tm1650_t *display, uint8_t level);
ecos_err_t ecos_tm1650_clear(ecos_tm1650_t *display);
ecos_err_t ecos_tm1650_set_segments(ecos_tm1650_t *display, uint8_t position,
                                    uint8_t segments);
ecos_err_t ecos_tm1650_show_digit(ecos_tm1650_t *display, uint8_t position,
                                  uint8_t value, bool dot);
ecos_err_t ecos_tm1650_show_number(ecos_tm1650_t *display, uint16_t value,
                                   bool leading_zero);
```

- `init` 配置 GPIO、清空所有位并以默认亮度点亮显示。
- `ecos_tm1650_set_brightness()`：`level` 0 关显示，1..8 选择占空比。
- `ecos_tm1650_set_segments()` 写裸段码：bit0=A … bit6=G，bit7=DP；
  `position` 0 = DIG1 … 3 = DIG4。
- `ecos_tm1650_show_digit()` 显示一位十六进制数字（0-15），`dot` 控制小数点。
- `ecos_tm1650_show_number()` 显示 0-9999 无符号十进制，右对齐；
  `leading_zero` 为真时高位补零，否则高位留空。

## 注意事项

- 所有 I2C 设备共用总线时，`ecos_i2c_init` 只调用一次，各设备驱动分别
  `ecos_<dev>_init`。
- 设备句柄（`ecos_<dev>_t`）由应用分配，可放静态区；不要在多任务/中断中
  并发访问同一句柄（SDK 为单线程裸机模型）。
- 显示与模组类驱动（ST7735/ST7789/ESP01S）内部会初始化其依赖的总线/引脚；
  与其共用一个控制器实例时要避免重复初始化。
- 需要 2.x 版 ST7735 行为的旧代码用的是同目录遗留头文件，不属于本页 API；
  新应用请用 `ecos/device/st7735.h`。
