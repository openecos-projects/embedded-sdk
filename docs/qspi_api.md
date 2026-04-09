# QSPI Driver API Documentation

## 概述

ECOS QSPI驱动提供了对四线串行外设接口（Quad Serial Peripheral Interface）的控制接口，支持高速数据传输。QSPI接口常用于连接Flash存储器、LCD显示屏等外设。

## 头文件

```c
#include "qspi.h"
```

## 数据类型

### qspi_config_t

QSPI配置结构体，用于初始化QSPI接口。

```c
typedef struct {
    uint32_t clkdiv;    ///< QSPI时钟分频值，sck = apb_clk/2(div+1)
} qspi_config_t;
```

## 寄存器定义

QSPI驱动提供了以下寄存器的直接访问：

```c
#define REG_CUST_QSPI_STATUS  (*(volatile uint32_t*)0x03007000)  // QSPI状态寄存器
#define REG_CUST_QSPI_CLKDIV  (*(volatile uint32_t*)0x03007004)  // QSPI时钟分频寄存器
#define REG_CUST_QSPI_CMD     (*(volatile uint32_t*)0x03007008)  // QSPI命令寄存器
#define REG_CUST_QSPI_ADR     (*(volatile uint32_t*)0x0300700c)  // QSPI地址寄存器
#define REG_CUST_QSPI_LEN     (*(volatile uint32_t*)0x03007010)  // QSPI长度寄存器
#define REG_CUST_QSPI_DUM     (*(volatile uint32_t*)0x03007014)  // QSPI虚拟周期寄存器
#define REG_CUST_QSPI_TXFIFO  (*(volatile uint32_t*)0x03007018)  // QSPI发送FIFO寄存器
#define REG_CUST_QSPI_RXFIFO  (*(volatile uint32_t*)0x03007020)  // QSPI接收FIFO寄存器
#define REG_CUST_QSPI_INTCFG  (*(volatile uint32_t*)0x03007024)  // QSPI中断配置寄存器
#define REG_CUST_QSPI_INTSTA  (*(volatile uint32_t*)0x03007028)  // QSPI中断状态寄存器
```

## API函数

### qspi_init()

**功能描述：** 初始化QSPI接口，配置时钟分频和基本参数。

**函数原型：**
```c
void qspi_init(qspi_config_t *config);
```

**参数：**
- `config`: 指向qspi_config_t结构体的指针，包含时钟分频配置

**返回值：** 无

**使用示例：**
```c
// 初始化QSPI，设置时钟分频为4
qspi_config_t qspi_conf = {
    .clkdiv = 4,  // sck = apb_clk/2(4+1) = apb_clk/10
};
qspi_init(&qspi_conf);
```

### qspi_write_8()

**功能描述：** 发送8位数据。

**函数原型：**
```c
void qspi_write_8(uint8_t data);
```

**参数：**
- `data`: 要发送的8位数据

**返回值：** 无

**使用示例：**
```c
// 发送8位命令
qspi_write_8(0x9F);  // 读取设备ID命令
```

### qspi_write_16()

**功能描述：** 发送16位数据。

**函数原型：**
```c
void qspi_write_16(uint16_t data);
```

**参数：**
- `data`: 要发送的16位数据

**返回值：** 无

**使用示例：**
```c
// 发送16位地址
qspi_write_16(0x1234);
```

### qspi_write_32()

**功能描述：** 发送32位数据。

**函数原型：**
```c
void qspi_write_32(uint32_t data);
```

**参数：**
- `data`: 要发送的32位数据

**返回值：** 无

**使用示例：**
```c
// 发送32位数据
qspi_write_32(0x12345678);
```

### qspi_write_32x2()

**功能描述：** 批量发送2个32位数据。

**函数原型：**
```c
void qspi_write_32x2(uint32_t data1, uint32_t data2);
```

**参数：**
- `data1`: 第1个32位数据
- `data2`: 第2个32位数据

**返回值：** 无

**使用示例：**
```c
// 批量发送2个32位数据
qspi_write_32x2(0x11111111, 0x22222222);
```

### qspi_write_32x8()

**功能描述：** 批量发送8个32位数据。

**函数原型：**
```c
void qspi_write_32x8(uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, 
                     uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8);
```

**参数：**
- `data1` ~ `data8`: 8个32位数据

**返回值：** 无

**使用示例：**
```c
// 批量发送8个32位数据
qspi_write_32x8(0x11111111, 0x22222222, 0x33333333, 0x44444444,
                0x55555555, 0x66666666, 0x77777777, 0x88888888);
```

### qspi_write_32x16()

**功能描述：** 批量发送16个32位数据。

**函数原型：**
```c
void qspi_write_32x16(uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, 
                      uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8,
                      uint32_t data9, uint32_t data10, uint32_t data11, uint32_t data12, 
                      uint32_t data13, uint32_t data14, uint32_t data15, uint32_t data16);
```

**参数：**
- `data1` ~ `data16`: 16个32位数据

**返回值：** 无

**使用示例：**
```c
// 批量发送16个32位数据
qspi_write_32x16(0x11111111, 0x22222222, 0x33333333, 0x44444444,
                 0x55555555, 0x66666666, 0x77777777, 0x88888888,
                 0x99999999, 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC,
                 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF, 0x00000000);
```

### qspi_write_32x32()

**功能描述：** 批量发送32个32位数据。

**函数原型：**
```c
void qspi_write_32x32(uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, 
                      uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8,
                      uint32_t data9, uint32_t data10, uint32_t data11, uint32_t data12, 
                      uint32_t data13, uint32_t data14, uint32_t data15, uint32_t data16,
                      uint32_t data17, uint32_t data18, uint32_t data19, uint32_t data20, 
                      uint32_t data21, uint32_t data22, uint32_t data23, uint32_t data24,
                      uint32_t data25, uint32_t data26, uint32_t data27, uint32_t data28, 
                      uint32_t data29, uint32_t data30, uint32_t data31, uint32_t data32);
```

**参数：**
- `data1` ~ `data32`: 32个32位数据

**返回值：** 无

**注意事项：** 该函数用于高速批量数据传输，常用于LCD显示等需要大量数据传输的场景。

**使用示例：**
```c
// 批量发送32个相同的颜色数据到LCD
uint32_t color = 0xFFFF0000;  // 红色
qspi_write_32x32(color, color, color, color, color, color, color, color,
                 color, color, color, color, color, color, color, color,
                 color, color, color, color, color, color, color, color,
                 color, color, color, color, color, color, color, color);
```

## 完整使用示例

### Flash存储器操作示例

```c
#include "qspi.h"

void flash_example(void)
{
    // 初始化QSPI
    qspi_config_t qspi_conf = {
        .clkdiv = 2,  // 设置合适的时钟分频
    };
    qspi_init(&qspi_conf);
    
    // 读取Flash设备ID
    qspi_write_8(0x9F);  // JEDEC ID命令
    
    // 写使能命令
    qspi_write_8(0x06);  // Write Enable命令
    
    // 页编程命令
    qspi_write_8(0x02);        // Page Program命令
    qspi_write_32(0x00000000); // 起始地址
    
    // 写入数据
    for (int i = 0; i < 64; i++) {
        qspi_write_8(i);
    }
}
```

### LCD显示数据传输示例

```c
#include "qspi.h"

void lcd_data_example(void)
{
    // 初始化QSPI
    qspi_config_t qspi_conf = {
        .clkdiv = 1,  // 高速传输
    };
    qspi_init(&qspi_conf);
    
    // 发送LCD命令
    qspi_write_8(0x2C);  // Memory Write命令
    
    // 批量发送像素数据（红色填充）
    uint32_t red_pixel = 0xF800F800;  // 16位红色像素数据
    
    // 使用批量传输提高效率
    for (int i = 0; i < 32; i++) {
        qspi_write_32x32(red_pixel, red_pixel, red_pixel, red_pixel,
                         red_pixel, red_pixel, red_pixel, red_pixel,
                         red_pixel, red_pixel, red_pixel, red_pixel,
                         red_pixel, red_pixel, red_pixel, red_pixel,
                         red_pixel, red_pixel, red_pixel, red_pixel,
                         red_pixel, red_pixel, red_pixel, red_pixel,
                         red_pixel, red_pixel, red_pixel, red_pixel,
                         red_pixel, red_pixel, red_pixel, red_pixel);
    }
}
```

## 注意事项

1. **时钟配置**：时钟分频值决定了QSPI的传输速度，公式为 `sck = apb_clk/2(div+1)`。需要根据外设的时钟要求选择合适的分频值。

2. **数据传输顺序**：QSPI驱动按照参数顺序依次发送数据，确保数据顺序正确。

3. **批量传输优势**：使用`qspi_write_32xN`系列函数可以提高传输效率，减少函数调用开销。

4. **硬件同步**：所有写入函数都会等待传输完成后返回，确保数据传输的可靠性。

5. **FIFO管理**：驱动内部自动管理TXFIFO，用户无需关心FIFO状态。

6. **中断支持**：当前版本的驱动使用轮询方式等待传输完成，未启用中断功能。

## 版本信息

- **版本**: v1.0
- **最后更新**: 2025年10月
- **许可证**: Apache-2.0