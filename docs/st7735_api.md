# ST7735 LCD Driver API Documentation

## 概述

ECOS ST7735驱动提供了对ST7735 TFT LCD显示屏的完整控制接口。ST7735是一款常用的小尺寸彩色LCD控制器，支持65K色显示，广泛应用于嵌入式显示系统。该驱动基于QSPI接口实现高速数据传输，并提供了完整的显示控制功能。

## 头文件

```c
#include "st7735.h"
```

## 数据类型

### st7735_device_t

ST7735设备配置结构体，用于初始化LCD显示屏。

```c
typedef struct {
    gpio_num_t dc_pin;           ///< 数据/命令选择引脚
    uint16_t screen_width;       ///< 屏幕宽度（像素）
    uint16_t screen_height;      ///< 屏幕高度（像素）
    uint8_t rotation;            ///< 屏幕旋转角度 (0, 1, 2, 3)
    uint8_t horizontal_offset;   ///< 水平偏移量
    uint8_t vertical_offset;     ///< 垂直偏移量
} st7735_device_t;
```

### 旋转角度定义

```c
#define ST7735_ROTATION_0    0    // 0度（正常方向）
#define ST7735_ROTATION_90   1    // 90度顺时针旋转
#define ST7735_ROTATION_180  2    // 180度旋转
#define ST7735_ROTATION_270  3    // 270度顺时针旋转
```

## API函数

### 底层数据传输函数

#### st7735_wr_cmd()

**功能描述：** 向ST7735发送命令。

**函数原型：**
```c
void st7735_wr_cmd(uint8_t cmd);
```

**参数：**
- `cmd`: 要发送的命令字节

**返回值：** 无

**使用示例：**
```c
// 发送显示开启命令
st7735_wr_cmd(0x29);
```

#### st7735_wr_data8()

**功能描述：** 向ST7735发送8位数据。

**函数原型：**
```c
void st7735_wr_data8(uint8_t data);
```

**参数：**
- `data`: 要发送的8位数据

**返回值：** 无

**使用示例：**
```c
// 发送8位参数数据
st7735_wr_data8(0x05);
```

#### st7735_wr_data16()

**功能描述：** 向ST7735发送16位数据。

**函数原型：**
```c
void st7735_wr_data16(uint16_t data);
```

**参数：**
- `data`: 要发送的16位数据

**返回值：** 无

**使用示例：**
```c
// 发送16位颜色数据（红色）
st7735_wr_data16(0xF800);
```

#### st7735_wr_data32()

**功能描述：** 向ST7735发送32位数据。

**函数原型：**
```c
void st7735_wr_data32(uint32_t data);
```

**参数：**
- `data`: 要发送的32位数据

**返回值：** 无

#### 批量数据传输函数

```c
void st7735_wr_data32x2(uint32_t data1, uint32_t data2);
void st7735_wr_data32x8(uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, 
                        uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8);
void st7735_wr_data32x16(uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, 
                         uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8,
                         uint32_t data9, uint32_t data10, uint32_t data11, uint32_t data12, 
                         uint32_t data13, uint32_t data14, uint32_t data15, uint32_t data16);
void st7735_wr_data32x32(uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, 
                         uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8,
                         uint32_t data9, uint32_t data10, uint32_t data11, uint32_t data12, 
                         uint32_t data13, uint32_t data14, uint32_t data15, uint32_t data16,
                         uint32_t data17, uint32_t data18, uint32_t data19, uint32_t data20, 
                         uint32_t data21, uint32_t data22, uint32_t data23, uint32_t data24,
                         uint32_t data25, uint32_t data26, uint32_t data27, uint32_t data28, 
                         uint32_t data29, uint32_t data30, uint32_t data31, uint32_t data32);
```

**功能描述：** 批量发送32位数据，用于高速像素数据传输。

### 高级控制函数

#### st7735_init()

**功能描述：** 初始化ST7735 LCD显示屏，包括硬件复位、寄存器配置和显示参数设置。

**函数原型：**
```c
void st7735_init(st7735_device_t *device);
```

**参数：**
- `device`: 指向st7735_device_t结构体的指针，包含显示屏配置参数

**返回值：** 无

**初始化流程：**
1. 配置DC控制引脚
2. 执行软件复位
3. 退出睡眠模式
4. 配置帧率控制
5. 设置电源管理
6. 配置显示方向和偏移
7. 设置颜色格式
8. 配置Gamma校正
9. 开启显示

**使用示例：**
```c
// 初始化128x160 ST7735显示屏
st7735_device_t lcd_device = {
    .dc_pin = GPIO_NUM_2,
    .screen_width = 128,
    .screen_height = 160,
    .rotation = ST7735_ROTATION_0,
    .horizontal_offset = 2,
    .vertical_offset = 1,
};
st7735_init(&lcd_device);
```

#### st7735_addr_set()

**功能描述：** 设置显示区域的地址窗口，定义后续像素数据的写入范围。

**函数原型：**
```c
void st7735_addr_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
```

**参数：**
- `x1`: 起始X坐标
- `y1`: 起始Y坐标  
- `x2`: 结束X坐标
- `y2`: 结束Y坐标

**返回值：** 无

**使用示例：**
```c
// 设置全屏显示区域
st7735_addr_set(0, 0, 127, 159);

// 设置部分区域
st7735_addr_set(10, 10, 50, 50);
```

#### st7735_fill()

**功能描述：** 用指定颜色填充指定的矩形区域。

**函数原型：**
```c
void st7735_fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
```

**参数：**
- `x1`: 起始X坐标
- `y1`: 起始Y坐标
- `x2`: 结束X坐标
- `y2`: 结束Y坐标
- `color`: 填充颜色（RGB565格式）

**返回值：** 无

**使用示例：**
```c
// 用红色填充全屏
st7735_fill(0, 0, 127, 159, 0xF800);

// 用蓝色填充矩形区域
st7735_fill(20, 20, 60, 60, 0x001F);
```

## 颜色定义

ST7735使用RGB565颜色格式，以下是常用颜色定义：

```c
#define ST7735_BLACK    0x0000    // 黑色
#define ST7735_WHITE    0xFFFF    // 白色
#define ST7735_RED      0xF800    // 红色
#define ST7735_GREEN    0x07E0    // 绿色
#define ST7735_BLUE     0x001F    // 蓝色
#define ST7735_YELLOW   0xFFE0    // 黄色
#define ST7735_MAGENTA  0xF81F    // 洋红色
#define ST7735_CYAN     0x07FF    // 青色
```

### RGB565颜色转换

```c
// RGB888转RGB565宏定义
#define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

// 使用示例
uint16_t orange = RGB565(255, 165, 0);  // 橙色
uint16_t purple = RGB565(128, 0, 128);  // 紫色
```

## 完整使用示例

### 基本显示示例

```c
#include "st7735.h"
#include "gpio.h"
#include "qspi.h"

void basic_display_example(void)
{
    // 初始化QSPI
    qspi_config_t qspi_conf = {
        .clkdiv = 1,  // 高速传输
    };
    qspi_init(&qspi_conf);
    
    // 初始化ST7735
    st7735_device_t lcd_device = {
        .dc_pin = GPIO_NUM_2,
        .screen_width = 128,
        .screen_height = 160,
        .rotation = ST7735_ROTATION_0,
        .horizontal_offset = 2,
        .vertical_offset = 1,
    };
    st7735_init(&lcd_device);
    
    // 清屏（黑色背景）
    st7735_fill(0, 0, 127, 159, ST7735_BLACK);
    
    // 绘制彩色矩形
    st7735_fill(10, 10, 50, 50, ST7735_RED);     // 红色矩形
    st7735_fill(60, 10, 100, 50, ST7735_GREEN);  // 绿色矩形
    st7735_fill(10, 60, 50, 100, ST7735_BLUE);   // 蓝色矩形
    st7735_fill(60, 60, 100, 100, ST7735_YELLOW); // 黄色矩形
}
```

### 像素级绘制示例

```c
#include "st7735.h"

void pixel_drawing_example(void)
{
    // 初始化显示屏（省略初始化代码）
    
    // 绘制渐变效果
    for (int y = 0; y < 160; y++) {
        for (int x = 0; x < 128; x++) {
            // 计算渐变颜色
            uint8_t red = (x * 255) / 128;
            uint8_t green = (y * 255) / 160;
            uint8_t blue = 128;
            
            uint16_t color = RGB565(red, green, blue);
            
            // 设置单像素地址并写入颜色
            st7735_addr_set(x, y, x, y);
            st7735_wr_data16(color);
        }
    }
}
```

### 图像显示示例

```c
#include "st7735.h"

// 显示位图图像
void display_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                   const uint16_t *bitmap)
{
    // 设置显示区域
    st7735_addr_set(x, y, x + width - 1, y + height - 1);
    
    // 批量传输像素数据
    uint32_t total_pixels = width * height;
    uint32_t i = 0;
    
    while (i < total_pixels) {
        if (total_pixels - i >= 32) {
            // 使用32x批量传输提高效率
            uint32_t data[32];
            for (int j = 0; j < 32; j++) {
                data[j] = (bitmap[i + j] << 16) | bitmap[i + j + 1];
                j++; i++;
            }
            st7735_wr_data32x32(data[0], data[1], data[2], data[3],
                               data[4], data[5], data[6], data[7],
                               data[8], data[9], data[10], data[11],
                               data[12], data[13], data[14], data[15],
                               data[16], data[17], data[18], data[19],
                               data[20], data[21], data[22], data[23],
                               data[24], data[25], data[26], data[27],
                               data[28], data[29], data[30], data[31]);
            i += 31;
        } else {
            // 剩余像素逐个传输
            st7735_wr_data16(bitmap[i]);
            i++;
        }
    }
}

void image_display_example(void)
{
    // 示例：显示64x64的彩色图像
    const uint16_t sample_image[64*64] = {
        // 图像数据（RGB565格式）
        ST7735_RED, ST7735_GREEN, ST7735_BLUE, ST7735_YELLOW,
        // ... 更多像素数据
    };
    
    // 在屏幕中央显示图像
    display_bitmap(32, 48, 64, 64, sample_image);
}
```

### 动画效果示例

```c
#include "st7735.h"
#include "timer.h"

void animation_example(void)
{
    uint16_t colors[] = {ST7735_RED, ST7735_GREEN, ST7735_BLUE, 
                        ST7735_YELLOW, ST7735_MAGENTA, ST7735_CYAN};
    int color_count = sizeof(colors) / sizeof(colors[0]);
    
    while (1) {
        for (int i = 0; i < color_count; i++) {
            // 全屏填充当前颜色
            st7735_fill(0, 0, 127, 159, colors[i]);
            delay_ms(500);  // 延时500ms
            
            // 绘制移动的矩形
            for (int x = 0; x < 100; x += 5) {
                // 清除之前的矩形
                st7735_fill(x-5, 50, x+23, 80, ST7735_BLACK);
                // 绘制新位置的矩形
                st7735_fill(x, 50, x+28, 80, ST7735_WHITE);
                delay_ms(50);
            }
        }
    }
}
```

### 文本显示框架示例

```c
#include "st7735.h"

// 简单的8x8点阵字符显示
void draw_char_8x8(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color)
{
    // 简化的8x8字符点阵数据（实际应用中需要完整的字库）
    const uint8_t font_8x8[][8] = {
        // 字符'A'的点阵数据示例
        {0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00},
        // ... 更多字符数据
    };
    
    if (c >= 'A' && c <= 'Z') {
        const uint8_t *char_data = font_8x8[c - 'A'];
        
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                uint16_t pixel_color = (char_data[row] & (0x80 >> col)) ? color : bg_color;
                st7735_addr_set(x + col, y + row, x + col, y + row);
                st7735_wr_data16(pixel_color);
            }
        }
    }
}

void text_display_example(void)
{
    // 清屏
    st7735_fill(0, 0, 127, 159, ST7735_BLACK);
    
    // 显示文本
    const char *text = "HELLO";
    uint16_t x = 10, y = 50;
    
    for (int i = 0; text[i] != '\0'; i++) {
        draw_char_8x8(x + i * 10, y, text[i], ST7735_WHITE, ST7735_BLACK);
    }
}
```

## 性能优化建议

### 批量数据传输

使用批量传输函数可以显著提高显示性能：

```c
// 低效方式：逐像素传输
for (int i = 0; i < 1000; i++) {
    st7735_wr_data16(pixel_data[i]);
}

// 高效方式：批量传输
for (int i = 0; i < 1000; i += 32) {
    // 使用st7735_wr_data32x32批量传输
    // 性能提升约10-20倍
}
```

### 地址窗口优化

合理设置地址窗口可以减少命令传输：

```c
// 设置一次地址窗口，然后连续写入像素数据
st7735_addr_set(0, 0, 127, 159);
for (int i = 0; i < total_pixels; i++) {
    st7735_wr_data16(frame_buffer[i]);
}
```

## 注意事项

1. **硬件连接**：确保DC引脚正确连接并在初始化前配置为输出模式。

2. **QSPI依赖**：ST7735驱动依赖QSPI接口，使用前必须先初始化QSPI。

3. **颜色格式**：ST7735使用RGB565颜色格式，注意颜色数据的字节序。

4. **坐标系统**：坐标原点(0,0)位于屏幕左上角，X轴向右，Y轴向下。

5. **旋转设置**：屏幕旋转会影响坐标系统和偏移量，需要相应调整应用逻辑。

6. **内存限制**：批量传输时注意栈空间限制，避免传递过大的数据数组。

7. **时序要求**：某些操作（如复位、睡眠退出）需要适当的延时，驱动已内置必要延时。

8. **电源管理**：长时间不使用时可以发送睡眠命令以降低功耗。

## 常见问题

### 显示异常

- **花屏**：检查QSPI时钟配置和硬件连接
- **颜色错误**：确认RGB565颜色格式转换正确
- **显示偏移**：调整horizontal_offset和vertical_offset参数

### 性能问题

- **刷新慢**：使用批量传输函数，优化QSPI时钟配置
- **闪烁**：避免频繁的全屏刷新，使用局部更新

### 初始化失败

- **无显示**：检查电源供应和复位时序
- **DC引脚**：确认DC引脚配置和连接正确

## 版本信息

- **版本**: v1.0
- **最后更新**: 2025年10月
- **许可证**: Apache-2.0
- **支持的ST7735变种**: ST7735R, ST7735S