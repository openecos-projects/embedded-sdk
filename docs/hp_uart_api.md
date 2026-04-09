# HP UART Driver API Documentation

## 概述

ECOS HP UART驱动提供了高性能UART串口通信接口，支持可配置波特率的串行数据传输。该驱动基于硬件寄存器直接操作，提供了字符和字符串的发送接收功能。

## 特性

- 支持可配置波特率
- 8位数据位，无校验位，1位停止位
- FIFO缓冲支持
- 字符和字符串收发
- 硬件状态检测
- 回显功能

## 头文件

```c
#include "hp_uart.h"
```

## 寄存器定义

### UART硬件寄存器

```c
#define REG_CUST_UART_LCR     (*(volatile uint32_t*)0x03003000)  // UART线路控制寄存器
#define REG_CUST_UART_DIV     (*(volatile uint32_t*)0x03003004)  // UART分频寄存器
#define REG_CUST_UART_TRX     (*(volatile uint32_t*)0x03003008)  // UART收发寄存器
#define REG_CUST_UART_FCR     (*(volatile uint32_t*)0x0300300c)  // UART FIFO控制寄存器
#define REG_CUST_UART_LSR     (*(volatile uint32_t*)0x03003010)  // UART线路状态寄存器
```

## API函数

### hp_uart_init

初始化HP UART模块。

**函数原型:**
```c
void hp_uart_init(uint32_t baudrate);
```

**参数:**
- `baudrate`: 波特率设置，常用值如9600、115200等

**返回值:**
- 无

**说明:**
- 配置UART波特率分频器
- 启用FIFO缓冲
- 设置8位数据位，无校验位，1位停止位
- 打印初始化信息到控制台

**示例:**
```c
#include "hp_uart.h"

int main() {
    // 初始化UART，波特率115200
    hp_uart_init(115200);
    
    return 0;
}
```

### hp_uart_send

发送单个字符。

**函数原型:**
```c
void hp_uart_send(char c);
```

**参数:**
- `c`: 要发送的字符

**返回值:**
- 无

**说明:**
- 等待发送缓冲区空闲
- 发送单个字符到UART
- 阻塞式发送，直到字符发送完成

**示例:**
```c
// 发送单个字符
hp_uart_send('A');
hp_uart_send('\n');
```

### hp_uart_send_str

发送字符串。

**函数原型:**
```c
void hp_uart_send_str(char* str);
```

**参数:**
- `str`: 指向要发送的以null结尾的字符串

**返回值:**
- 无

**说明:**
- 逐个字符发送整个字符串
- 自动处理字符串结束符
- 阻塞式发送

**示例:**
```c
// 发送字符串
hp_uart_send_str("Hello, World!\n");
hp_uart_send_str("UART Test Message");
```

### hp_uart_recv

接收单个字符。

**函数原型:**
```c
void hp_uart_recv(char* c);
```

**参数:**
- `c`: 指向存储接收字符的变量

**返回值:**
- 无

**说明:**
- 等待接收缓冲区有数据
- 接收单个字符
- 自动回显接收到的字符
- 阻塞式接收

**示例:**
```c
char received_char;

// 接收单个字符
hp_uart_recv(&received_char);
printf("Received: %c\n", received_char);
```

### hp_uart_recv_str

接收字符串。

**函数原型:**
```c
void hp_uart_recv_str(char* str);
```

**参数:**
- `str`: 指向存储接收字符串的缓冲区

**返回值:**
- 无

**说明:**
- 连续接收字符直到遇到换行符('\n')
- 自动在换行符处结束接收
- 包含回显功能
- 阻塞式接收

**示例:**
```c
char buffer[100];

// 接收字符串（直到换行符）
hp_uart_recv_str(buffer);
printf("Received string: %s", buffer);
```

## 使用流程

### 基本使用流程

```c
#include "hp_uart.h"
#include <stdio.h>

int main() {
    char buffer[100];
    char ch;
    
    // 1. 初始化UART
    hp_uart_init(115200);
    
    // 2. 发送欢迎信息
    hp_uart_send_str("HP UART Test Program\n");
    hp_uart_send_str("Enter a character: ");
    
    // 3. 接收单个字符
    hp_uart_recv(&ch);
    
    // 4. 发送确认信息
    hp_uart_send_str("\nYou entered: ");
    hp_uart_send(ch);
    hp_uart_send('\n');
    
    // 5. 接收字符串
    hp_uart_send_str("Enter a string (end with Enter): ");
    hp_uart_recv_str(buffer);
    
    // 6. 发送接收到的字符串
    hp_uart_send_str("You entered: ");
    hp_uart_send_str(buffer);
    
    return 0;
}
```

## 注意事项

### 使用注意事项

1. **初始化顺序**: 必须先调用`hp_uart_init()`初始化UART后才能进行收发操作
2. **阻塞特性**: 所有收发函数都是阻塞式的，会等待操作完成
3. **缓冲区大小**: 接收字符串时确保提供足够大的缓冲区
4. **回显功能**: `hp_uart_recv()`和`hp_uart_recv_str()`会自动回显接收到的字符
5. **字符串结束**: `hp_uart_recv_str()`以换行符('\n')作为字符串结束标志

### 波特率配置

- 波特率计算公式: `DIV = (CPU_FREQ / baudrate) - 1`
- 常用波特率: 9600, 19200, 38400, 57600, 115200
- 确保发送端和接收端波特率一致

### 硬件配置

- 数据位: 8位
- 校验位: 无
- 停止位: 1位
- 流控制: 无

## 故障排除

### 常见问题

1. **无法发送数据**
   - 检查UART是否正确初始化
   - 确认波特率设置正确
   - 检查硬件连接

2. **接收数据异常**
   - 确认发送端波特率匹配
   - 检查数据格式设置
   - 验证硬件连接

3. **字符乱码**
   - 检查波特率是否匹配
   - 确认数据位、校验位、停止位设置
   - 检查时钟频率配置

## 版本信息

- **版本**: 1.0.0
- **更新日期**: 2025年10月

## 相关文档
- [开发指南](development_guide.md)