# I2C Driver API Documentation

## 概述

ECOS I2C驱动提供了对I2C（Inter-Integrated Circuit）总线的完整控制接口。I2C是一种串行通信协议，广泛用于连接各种传感器、存储器、显示器等外设。该驱动支持主机模式操作，提供了读写操作的高级接口。

## 头文件

```c
#include "i2c.h"
```

## 数据类型

### i2c_config_t

I2C配置结构体，用于初始化I2C接口。

```c
typedef struct {
    uint32_t pscr;    ///< I2C预分频值，用于设置I2C时钟频率
} i2c_config_t;
```

### i2c_reg_addr_len_t

I2C寄存器地址长度枚举，定义目标设备寄存器地址的位宽。

```c
typedef enum {
    I2C_REG_8 = 8,     ///< 8位寄存器地址
    I2C_REG_16 = 16,   ///< 16位寄存器地址
} i2c_reg_addr_len_t;
```

### i2c_cmd_t

I2C命令枚举，定义各种I2C操作命令。

```c
typedef enum {
    I2C_START = 0x80,        ///< 发送START信号
    I2C_STOP = 0x40,         ///< 发送STOP信号
    I2C_READ = 0x10,         ///< 读取数据
    I2C_WRITE = 0x20,        ///< 写入数据
    I2C_START_READ = 0xA0,   ///< START + 读取
    I2C_START_WRITE = 0x90,  ///< START + 写入
    I2C_STOP_READ = 0x60,    ///< STOP + 读取
    I2C_STOP_WRITE = 0x50,   ///< STOP + 写入
} i2c_cmd_t;
```

## 寄存器定义

I2C驱动提供了以下寄存器的直接访问：

```c
#define REG_CUST_I2C_CTRL  (*(volatile uint32_t*)0x03006000)  // I2C控制寄存器
#define REG_CUST_I2C_PSCR  (*(volatile uint32_t*)0x03006004)  // I2C预分频寄存器
#define REG_CUST_I2C_TXR   (*(volatile uint32_t*)0x03006008)  // I2C发送寄存器
#define REG_CUST_I2C_RXR   (*(volatile uint32_t*)0x0300600c)  // I2C接收寄存器
#define REG_CUST_I2C_CMD   (*(volatile uint32_t*)0x03006010)  // I2C命令寄存器
#define REG_CUST_I2C_SR    (*(volatile uint32_t*)0x03006014)  // I2C状态寄存器
```

### 状态寄存器位定义

```c
#define I2C_STATUS_RXACK  0x80  // 接收ACK状态位
#define I2C_STATUS_BUSY   0x40  // 总线忙状态位
#define I2C_STATUS_AL     0x20  // 仲裁丢失状态位
#define I2C_STATUS_TIP    0x02  // 传输进行中状态位
#define I2C_STATUS_IF     0x01  // 中断标志位
```

## API函数

### i2c_init()

**功能描述：** 初始化I2C接口，配置时钟分频和启用I2C控制器。

**函数原型：**
```c
void i2c_init(i2c_config_t* config);
```

**参数：**
- `config`: 指向i2c_config_t结构体的指针，包含预分频配置

**返回值：** 无

**工作原理：**
1. 禁用I2C控制器
2. 设置预分频值
3. 启用I2C控制器

**使用示例：**
```c
// 初始化I2C，设置预分频为100（适用于100kHz标准速度）
i2c_config_t i2c_conf = {
    .pscr = 100,  // 预分频值，影响I2C时钟频率
};
i2c_init(&i2c_conf);
```

### i2c_write_nbyte()

**功能描述：** 向I2C从设备的指定寄存器写入多个字节数据。

**函数原型：**
```c
void i2c_write_nbyte(uint8_t slave_addr, uint16_t reg_addr, 
                     i2c_reg_addr_len_t reg_addr_len, uint8_t* data, uint32_t len);
```

**参数：**
- `slave_addr`: I2C从设备地址（7位地址，不包含读写位）
- `reg_addr`: 目标寄存器地址
- `reg_addr_len`: 寄存器地址长度（8位或16位）
- `data`: 指向要写入数据的缓冲区
- `len`: 要写入的字节数

**返回值：** 无

**操作流程：**
1. 发送START信号和从设备写地址
2. 发送寄存器地址（8位或16位）
3. 依次发送数据字节
4. 发送STOP信号

**使用示例：**
```c
// 向地址为0x50的EEPROM写入数据
uint8_t write_data[] = {0x12, 0x34, 0x56, 0x78};
i2c_write_nbyte(0x50, 0x0000, I2C_REG_16, write_data, 4);

// 向地址为0x68的RTC芯片写入时间数据
uint8_t time_data[] = {0x00, 0x30, 0x14};  // 秒、分、时
i2c_write_nbyte(0x68, 0x00, I2C_REG_8, time_data, 3);
```

### i2c_read_nbyte()

**功能描述：** 从I2C从设备的指定寄存器读取多个字节数据。

**函数原型：**
```c
void i2c_read_nbyte(uint8_t slave_addr, uint16_t reg_addr, 
                    i2c_reg_addr_len_t reg_addr_len, uint8_t* data, uint32_t len);
```

**参数：**
- `slave_addr`: I2C从设备地址（7位地址，不包含读写位）
- `reg_addr`: 目标寄存器地址
- `reg_addr_len`: 寄存器地址长度（8位或16位）
- `data`: 指向接收数据的缓冲区
- `len`: 要读取的字节数

**返回值：** 无

**操作流程：**
1. 发送START信号和从设备写地址
2. 发送寄存器地址（8位或16位）
3. 发送STOP信号
4. 发送START信号和从设备读地址
5. 依次读取数据字节
6. 发送STOP信号

**使用示例：**
```c
// 从地址为0x50的EEPROM读取数据
uint8_t read_data[4];
i2c_read_nbyte(0x50, 0x0000, I2C_REG_16, read_data, 4);

// 从地址为0x68的RTC芯片读取时间数据
uint8_t time_data[3];
i2c_read_nbyte(0x68, 0x00, I2C_REG_8, time_data, 3);
printf("Time: %02d:%02d:%02d\n", time_data[2], time_data[1], time_data[0]);
```

## 完整使用示例

### EEPROM读写示例

```c
#include "i2c.h"
#include <stdio.h>

void eeprom_example(void)
{
    // 初始化I2C
    i2c_config_t i2c_conf = {
        .pscr = 100,  // 100kHz标准速度
    };
    i2c_init(&i2c_conf);
    
    // EEPROM设备地址（AT24C256）
    uint8_t eeprom_addr = 0x50;
    uint16_t mem_addr = 0x0100;  // 内存地址
    
    // 写入数据到EEPROM
    uint8_t write_buffer[] = "Hello I2C!";
    uint32_t data_len = sizeof(write_buffer) - 1;  // 不包含字符串结束符
    
    printf("Writing to EEPROM...\n");
    i2c_write_nbyte(eeprom_addr, mem_addr, I2C_REG_16, write_buffer, data_len);
    
    // 等待EEPROM写入完成（通常需要5ms）
    delay_ms(10);
    
    // 从EEPROM读取数据
    uint8_t read_buffer[32];
    printf("Reading from EEPROM...\n");
    i2c_read_nbyte(eeprom_addr, mem_addr, I2C_REG_16, read_buffer, data_len);
    
    // 添加字符串结束符并打印
    read_buffer[data_len] = '\0';
    printf("Read data: %s\n", read_buffer);
}
```

### RTC时钟芯片示例

```c
#include "i2c.h"
#include <stdio.h>

// BCD码转换函数
uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

void rtc_example(void)
{
    // 初始化I2C
    i2c_config_t i2c_conf = {
        .pscr = 100,  // 100kHz
    };
    i2c_init(&i2c_conf);
    
    // DS1307 RTC芯片地址
    uint8_t rtc_addr = 0x68;
    
    // 设置时间：14:30:00
    uint8_t time_set[] = {
        dec_to_bcd(0),   // 秒
        dec_to_bcd(30),  // 分
        dec_to_bcd(14),  // 时（24小时制）
    };
    
    printf("Setting RTC time to 14:30:00\n");
    i2c_write_nbyte(rtc_addr, 0x00, I2C_REG_8, time_set, 3);
    
    // 读取当前时间
    while (1) {
        uint8_t time_read[3];
        i2c_read_nbyte(rtc_addr, 0x00, I2C_REG_8, time_read, 3);
        
        uint8_t seconds = bcd_to_dec(time_read[0] & 0x7F);  // 清除CH位
        uint8_t minutes = bcd_to_dec(time_read[1]);
        uint8_t hours = bcd_to_dec(time_read[2] & 0x3F);    // 24小时制
        
        printf("Current time: %02d:%02d:%02d\n", hours, minutes, seconds);
        delay_ms(1000);  // 每秒更新一次
    }
}
```

### 温度传感器示例

```c
#include "i2c.h"
#include <stdio.h>

void temperature_sensor_example(void)
{
    // 初始化I2C
    i2c_config_t i2c_conf = {
        .pscr = 100,
    };
    i2c_init(&i2c_conf);
    
    // LM75温度传感器地址
    uint8_t sensor_addr = 0x48;
    
    while (1) {
        // 读取温度数据（2字节）
        uint8_t temp_data[2];
        i2c_read_nbyte(sensor_addr, 0x00, I2C_REG_8, temp_data, 2);
        
        // 转换温度值（LM75格式：11位，0.125°C分辨率）
        int16_t temp_raw = (temp_data[0] << 8) | temp_data[1];
        temp_raw >>= 5;  // 右移5位得到11位温度值
        
        float temperature = temp_raw * 0.125f;
        
        printf("Temperature: %.2f°C\n", temperature);
        delay_ms(2000);  // 每2秒读取一次
    }
}
```

### I2C设备扫描示例

```c
#include "i2c.h"
#include <stdio.h>

void i2c_scan_devices(void)
{
    // 初始化I2C
    i2c_config_t i2c_conf = {
        .pscr = 100,
    };
    i2c_init(&i2c_conf);
    
    printf("Scanning I2C bus...\n");
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    
    for (uint8_t row = 0; row < 8; row++) {
        printf("%02x: ", row * 16);
        
        for (uint8_t col = 0; col < 16; col++) {
            uint8_t addr = row * 16 + col;
            
            if (addr < 0x08 || addr > 0x77) {
                printf("   ");  // 无效地址范围
            } else {
                // 尝试与设备通信
                uint8_t dummy_data = 0;
                
                // 简单的设备检测：尝试写入操作
                REG_CUST_I2C_TXR = addr << 1;  // 写地址
                REG_CUST_I2C_CMD = I2C_START_WRITE;
                
                // 等待操作完成
                while((REG_CUST_I2C_SR & I2C_STATUS_TIP) == 0);
                while((REG_CUST_I2C_SR & I2C_STATUS_TIP) != 0);
                
                // 发送STOP信号
                REG_CUST_I2C_CMD = I2C_STOP;
                while((REG_CUST_I2C_SR & I2C_STATUS_BUSY) == I2C_STATUS_BUSY);
                
                // 检查是否收到ACK
                if (!(REG_CUST_I2C_SR & I2C_STATUS_RXACK)) {
                    printf("%02x ", addr);  // 设备响应
                } else {
                    printf("-- ");  // 无设备响应
                }
            }
        }
        printf("\n");
    }
    printf("Scan complete.\n");
}
```

## 时钟配置

### 预分频计算

I2C时钟频率的计算公式：
```
I2C_CLK = CPU_CLK / (5 * (pscr + 1))
```

常用配置示例：
```c
// 假设CPU时钟为72MHz
// 标准模式 (100kHz)
i2c_config_t std_config = { .pscr = 143 };  // 72MHz / (5 * 144) = 100kHz

// 快速模式 (400kHz)  
i2c_config_t fast_config = { .pscr = 35 };  // 72MHz / (5 * 36) = 400kHz

// 低速模式 (10kHz)
i2c_config_t slow_config = { .pscr = 1439 }; // 72MHz / (5 * 1440) = 10kHz
```

## 错误处理

驱动内部包含基本的错误检测和打印：

```c
// ACK检测失败时的错误信息
"I2C: start write no ack"  // 设备地址无响应
"I2C: start read no ack"   // 读操作无响应  
"I2C: write byte no ack"   // 数据写入无响应
"I2C: read byte no ack"    // 数据读取无响应
```

### 常见错误处理策略

```c
void robust_i2c_write(uint8_t addr, uint16_t reg, uint8_t* data, uint32_t len)
{
    const int max_retries = 3;
    
    for (int retry = 0; retry < max_retries; retry++) {
        // 尝试写入操作
        i2c_write_nbyte(addr, reg, I2C_REG_8, data, len);
        
        // 验证写入（如果设备支持回读）
        uint8_t verify_data[len];
        delay_ms(1);  // 等待写入完成
        i2c_read_nbyte(addr, reg, I2C_REG_8, verify_data, len);
        
        // 比较数据
        bool success = true;
        for (uint32_t i = 0; i < len; i++) {
            if (data[i] != verify_data[i]) {
                success = false;
                break;
            }
        }
        
        if (success) {
            printf("I2C write successful\n");
            return;
        } else {
            printf("I2C write failed, retry %d/%d\n", retry + 1, max_retries);
            delay_ms(10);  // 重试前等待
        }
    }
    
    printf("I2C write failed after %d retries\n", max_retries);
}
```

## 性能特性

### 传输速度

- **标准模式**: 100kHz（推荐用于大多数应用）
- **快速模式**: 400kHz（适用于高速传输需求）
- **低速模式**: 10kHz（适用于长距离或噪声环境）

### 数据容量

- **单次传输**: 理论上无限制，受内存和设备限制
- **寄存器地址**: 支持8位和16位地址
- **设备地址**: 支持7位I2C地址（0x08-0x77）

## 注意事项

1. **设备地址格式**：函数参数中的slave_addr是7位地址，驱动内部会自动添加读写位。

2. **寄存器地址字节序**：16位寄存器地址采用大端序（高字节在前）。

3. **ACK检测**：所有操作都包含ACK检测，失败时会打印错误信息。

4. **总线状态**：驱动会等待总线空闲后才发送STOP信号。

5. **时钟配置**：预分频值需要根据CPU时钟和目标I2C频率计算。

6. **设备特性**：不同I2C设备可能有特殊的时序要求，请参考设备数据手册。

7. **多主机模式**：当前驱动仅支持主机模式，不支持多主机仲裁。

8. **中断支持**：当前版本使用轮询方式，未实现中断驱动模式。

## 兼容设备

该I2C驱动兼容标准I2C协议的设备，包括但不限于：

- **存储器**: EEPROM (AT24Cxx系列)、FRAM
- **时钟芯片**: DS1307、DS3231、PCF8563
- **传感器**: LM75温度传感器、MPU6050陀螺仪、BMP280气压传感器
- **显示器**: OLED显示屏（SSD1306）、LCD显示模块
- **IO扩展**: PCF8574、MCP23017
- **ADC/DAC**: ADS1115、MCP4725

## 版本信息

- **版本**: v1.0
- **最后更新**: 2025年10月
- **许可证**: Apache-2.0