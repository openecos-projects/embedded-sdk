# GPIO Driver API Documentation

## 概述

ECOS GPIO驱动提供了对通用输入输出引脚的控制接口，支持16个GPIO引脚（GPIO_NUM_0 到 GPIO_NUM_15）的配置和操作。

## 头文件

```c
#include "gpio.h"
```

## 数据类型

### gpio_num_t

GPIO引脚编号枚举类型，定义了可用的GPIO引脚。

```c
typedef enum {
    GPIO_NUM_0 = 0,     ///< GPIO引脚0
    GPIO_NUM_1 = 1,     ///< GPIO引脚1
    GPIO_NUM_2 = 2,     ///< GPIO引脚2
    GPIO_NUM_3 = 3,     ///< GPIO引脚3
    GPIO_NUM_4 = 4,     ///< GPIO引脚4
    GPIO_NUM_5 = 5,     ///< GPIO引脚5
    GPIO_NUM_6 = 6,     ///< GPIO引脚6
    GPIO_NUM_7 = 7,     ///< GPIO引脚7
    GPIO_NUM_8 = 8,     ///< GPIO引脚8
    GPIO_NUM_9 = 9,     ///< GPIO引脚9
    GPIO_NUM_10 = 10,   ///< GPIO引脚10
    GPIO_NUM_11 = 11,   ///< GPIO引脚11
    GPIO_NUM_12 = 12,   ///< GPIO引脚12
    GPIO_NUM_13 = 13,   ///< GPIO引脚13
    GPIO_NUM_14 = 14,   ///< GPIO引脚14
    GPIO_NUM_15 = 15,   ///< GPIO引脚15
} gpio_num_t;
```

### gpio_mode_t

GPIO工作模式枚举类型。

```c
typedef enum {
    GPIO_MODE_INPUT = 0,    ///< 输入模式
    GPIO_MODE_OUTPUT = 1,   ///< 输出模式
} gpio_mode_t;
```

### gpio_level_t

GPIO电平状态枚举类型。

```c
typedef enum {
    GPIO_LEVEL_LOW = 0,     ///< 低电平
    GPIO_LEVEL_HIGH = 1,    ///< 高电平
} gpio_level_t;
```

### gpio_config_t

GPIO配置结构体，用于批量配置GPIO引脚。

```c
typedef struct {
    uint64_t pin_bit_mask;  ///< 引脚位掩码，用于设置或读取多个引脚
    gpio_mode_t mode;       ///< GPIO模式，输入或输出
} gpio_config_t;
```

## API函数

### gpio_config()

**功能描述：** 根据配置结构体批量配置GPIO引脚的工作模式。

**函数原型：**
```c
void gpio_config(const gpio_config_t* config);
```

**参数：**
- `config`: 指向gpio_config_t结构体的指针，包含引脚位掩码和模式信息

**返回值：** 无

**使用示例：**
```c
// 配置GPIO0和GPIO1为输出模式
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_0) | (1ULL << GPIO_NUM_1),
    .mode = GPIO_MODE_OUTPUT,
};
gpio_config(&io_conf);

// 配置GPIO2为输入模式
gpio_config_t input_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_2),
    .mode = GPIO_MODE_INPUT,
};
gpio_config(&input_conf);
```

### gpio_set_level()

**功能描述：** 设置指定GPIO引脚的输出电平。

**函数原型：**
```c
void gpio_set_level(gpio_num_t gpio_num, gpio_level_t level);
```

**参数：**
- `gpio_num`: GPIO引脚号，类型为gpio_num_t
- `level`: 要设置的电平，类型为gpio_level_t

**返回值：** 无

**注意事项：** 
- 该函数仅对配置为输出模式的GPIO引脚有效
- 调用前需要先使用gpio_config()将引脚配置为输出模式

**使用示例：**
```c
// 设置GPIO0输出高电平
gpio_set_level(GPIO_NUM_0, GPIO_LEVEL_HIGH);

// 设置GPIO1输出低电平
gpio_set_level(GPIO_NUM_1, GPIO_LEVEL_LOW);
```

### gpio_get_level()

**功能描述：** 读取指定GPIO引脚的当前电平状态。

**函数原型：**
```c
int32_t gpio_get_level(gpio_num_t gpio_num);
```

**参数：**
- `gpio_num`: GPIO引脚号，类型为gpio_num_t

**返回值：**
- `0`: 低电平
- `1`: 高电平

**注意事项：**
- 该函数可以读取输入模式或输出模式的GPIO引脚状态
- 对于输入模式的引脚，读取外部输入的电平
- 对于输出模式的引脚，读取当前输出的电平

**使用示例：**
```c
// 读取GPIO2的电平状态
int32_t level = gpio_get_level(GPIO_NUM_2);
if (level == 1) {
    printf("GPIO2 is HIGH\n");
} else {
    printf("GPIO2 is LOW\n");
}

// 检测按键状态（假设按键连接到GPIO3，按下时为低电平）
if (gpio_get_level(GPIO_NUM_3) == 0) {
    printf("Button pressed\n");
}
```

## 完整使用示例

### LED控制示例

```c
#include "gpio.h"

void led_example(void)
{
    // 配置GPIO0为输出模式，用于控制LED
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_0),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&led_conf);
    
    // LED闪烁
    for (int i = 0; i < 10; i++) {
        gpio_set_level(GPIO_NUM_0, GPIO_LEVEL_HIGH);  // 点亮LED
        delay_ms(500);
        gpio_set_level(GPIO_NUM_0, GPIO_LEVEL_LOW);   // 熄灭LED
        delay_ms(500);
    }
}
```

### 按键检测示例

```c
#include "gpio.h"

void button_example(void)
{
    // 配置GPIO1为输入模式，用于检测按键
    gpio_config_t button_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_1),
        .mode = GPIO_MODE_INPUT,
    };
    gpio_config(&button_conf);
    
    // 配置GPIO0为输出模式，用于控制LED
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_0),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&led_conf);
    
    // 按键控制LED
    while (1) {
        if (gpio_get_level(GPIO_NUM_1) == 0) {  // 按键按下（低电平）
            gpio_set_level(GPIO_NUM_0, GPIO_LEVEL_HIGH);  // 点亮LED
        } else {
            gpio_set_level(GPIO_NUM_0, GPIO_LEVEL_LOW);   // 熄灭LED
        }
        delay_ms(10);  // 防抖延时
    }
}
```

### 多引脚批量配置示例

```c
#include "gpio.h"

void multi_gpio_example(void){
    // 批量配置GPIO0-3为输出模式
    gpio_config_t output_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_0) | (1ULL << GPIO_NUM_1) | 
                       (1ULL << GPIO_NUM_2) | (1ULL << GPIO_NUM_3),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&output_conf);
    
    // 批量配置GPIO4-7为输入模式
    gpio_config_t input_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_4) | (1ULL << GPIO_NUM_5) | 
                       (1ULL << GPIO_NUM_6) | (1ULL << GPIO_NUM_7),
        .mode = GPIO_MODE_INPUT,
    };
    gpio_config(&input_conf);
    
    // 流水灯效果
    while (1) {
        for (int i = 0; i < 4; i++) {
            gpio_set_level(i, GPIO_LEVEL_HIGH);
            delay_ms(200);
            gpio_set_level(i, GPIO_LEVEL_LOW);
        }
    }
}
```

## 注意事项

1. **引脚配置顺序**：在使用gpio_set_level()之前，必须先调用gpio_config()配置引脚为输出模式。

2. **位掩码使用**：在gpio_config_t结构体中，pin_bit_mask使用64位无符号整数，每一位对应一个GPIO引脚。

3. **硬件限制**：当前驱动支持16个GPIO引脚（GPIO_NUM_0到GPIO_NUM_15），具体可用引脚数量取决于硬件设计。

4. **电平读取**：gpio_get_level()函数可以读取任何配置过的GPIO引脚的电平状态，无论是输入还是输出模式。

5. **线程安全**：当前驱动实现不包含线程安全保护，在多线程环境中使用时需要额外的同步机制。

## 版本信息

- **版本**: v1.0
- **最后更新**: 2025年10月
- **许可证**: Apache-2.0