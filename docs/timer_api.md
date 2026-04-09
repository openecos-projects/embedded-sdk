# Timer Driver API Documentation

## 概述

ECOS Timer驱动提供了基于硬件定时器的延时功能，支持毫秒级精确延时。该驱动基于TIM0硬件定时器实现，适用于需要精确时间控制的应用场景。

## 头文件

```c
#include "timer.h"
```

## 寄存器定义

Timer驱动提供了以下寄存器的直接访问：

```c
#define REG_TIM0_CONFIG  (*(volatile uint32_t*)0x03005000)  // TIM0配置寄存器
#define REG_TIM0_VALUE   (*(volatile uint32_t*)0x03005004)  // TIM0当前值寄存器
#define REG_TIM0_DATA    (*(volatile uint32_t*)0x03005008)  // TIM0数据寄存器
```

### 寄存器说明

- **REG_TIM0_CONFIG**: 定时器配置寄存器，用于控制定时器的启动和停止
- **REG_TIM0_VALUE**: 定时器当前计数值寄存器，只读
- **REG_TIM0_DATA**: 定时器目标值寄存器，设置定时器的计数目标

## API函数

### delay_ms()

**功能描述：** 提供毫秒级精确延时功能。

**函数原型：**
```c
void delay_ms(uint32_t ms);
```

**参数：**
- `ms`: 延时时间，单位为毫秒

**返回值：** 无

**工作原理：**
该函数基于系统时钟频率（CONFIG_CPU_FREQ_MHZ）计算定时器计数值，通过硬件定时器实现精确延时。计算公式为：
```
timer_count = ms * CONFIG_CPU_FREQ_MHZ * 1000
```

**使用示例：**
```c
#include "timer.h"

// 延时100毫秒
delay_ms(100);

// 延时1秒
delay_ms(1000);

// 延时500微秒（0.5毫秒）
delay_ms(1);  // 最小延时单位为1毫秒
```

## 完整使用示例

### LED闪烁示例

```c
#include "timer.h"
#include "gpio.h"

void led_blink_example(void)
{
    // 配置LED GPIO
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_config);
    
    // LED闪烁循环
    while (1) {
        gpio_set_level(GPIO_NUM_2, 1);  // 点亮LED
        delay_ms(500);                  // 延时500毫秒
        
        gpio_set_level(GPIO_NUM_2, 0);  // 熄灭LED
        delay_ms(500);                  // 延时500毫秒
    }
}
```

### 按键防抖示例

```c
#include "timer.h"
#include "gpio.h"

uint8_t button_debounce_read(gpio_num_t button_pin)
{
    static uint8_t last_state = 1;  // 假设按键默认为高电平
    uint8_t current_state;
    
    current_state = gpio_get_level(button_pin);
    
    if (current_state != last_state) {
        delay_ms(20);  // 防抖延时20毫秒
        current_state = gpio_get_level(button_pin);
        
        if (current_state != last_state) {
            last_state = current_state;
            return current_state;
        }
    }
    
    return last_state;
}

void button_example(void)
{
    // 配置按键GPIO
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_0),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&button_config);
    
    while (1) {
        if (button_debounce_read(GPIO_NUM_0) == 0) {  // 按键按下
            printf("Button pressed!\n");
            delay_ms(100);  // 避免重复检测
        }
        delay_ms(10);  // 主循环延时
    }
}
```

### 定时任务调度示例

```c
#include "timer.h"
#include <stdio.h>

void task_scheduler_example(void)
{
    uint32_t task1_counter = 0;
    uint32_t task2_counter = 0;
    uint32_t task3_counter = 0;
    
    while (1) {
        // 任务1：每100ms执行一次
        if (task1_counter >= 100) {
            printf("Task 1 executed\n");
            task1_counter = 0;
        }
        
        // 任务2：每500ms执行一次
        if (task2_counter >= 500) {
            printf("Task 2 executed\n");
            task2_counter = 0;
        }
        
        // 任务3：每1000ms执行一次
        if (task3_counter >= 1000) {
            printf("Task 3 executed\n");
            task3_counter = 0;
        }
        
        // 基础时间片：10ms
        delay_ms(10);
        task1_counter += 10;
        task2_counter += 10;
        task3_counter += 10;
    }
}
```

### 超时检测示例

```c
#include "timer.h"
#include "gpio.h"

typedef enum {
    TIMEOUT_OK,
    TIMEOUT_EXPIRED
} timeout_result_t;

timeout_result_t wait_for_signal_with_timeout(gpio_num_t signal_pin, 
                                              uint32_t timeout_ms)
{
    uint32_t elapsed_time = 0;
    const uint32_t check_interval = 1;  // 每1ms检查一次
    
    while (elapsed_time < timeout_ms) {
        if (gpio_get_level(signal_pin) == 1) {  // 检测到信号
            return TIMEOUT_OK;
        }
        
        delay_ms(check_interval);
        elapsed_time += check_interval;
    }
    
    return TIMEOUT_EXPIRED;  // 超时
}

void timeout_example(void)
{
    // 配置信号GPIO
    gpio_config_t signal_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_1),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&signal_config);
    
    printf("Waiting for signal...\n");
    
    timeout_result_t result = wait_for_signal_with_timeout(GPIO_NUM_1, 5000);  // 5秒超时
    
    if (result == TIMEOUT_OK) {
        printf("Signal detected!\n");
    } else {
        printf("Timeout: No signal detected within 5 seconds\n");
    }
}
```

## 性能特性

### 延时精度

- **最小延时**: 1毫秒
- **精度**: 基于系统时钟，精度取决于CONFIG_CPU_FREQ_MHZ配置
- **最大延时**: 理论上支持uint32_t范围内的任意毫秒值（约49.7天）

### 资源占用

- **硬件资源**: 使用TIM0定时器
- **内存占用**: 函数调用时仅占用栈空间，无静态内存占用
- **CPU占用**: 延时期间CPU处于等待状态，不执行其他任务

## 注意事项

1. **阻塞特性**：`delay_ms()`是阻塞函数，在延时期间CPU无法执行其他任务。

2. **时钟依赖**：延时精度依赖于系统时钟配置（CONFIG_CPU_FREQ_MHZ），确保该配置与实际硬件时钟频率一致。

3. **硬件资源**：该驱动独占使用TIM0定时器，其他代码不应直接操作TIM0相关寄存器。

4. **中断影响**：在延时期间，中断仍然可以正常响应，但会影响延时的精确性。

5. **嵌套调用**：支持在中断服务程序中调用，但需要注意中断嵌套可能带来的时序问题。

6. **最小延时限制**：由于硬件和软件开销，实际最小有效延时可能略大于1毫秒。

7. **功耗考虑**：在低功耗应用中，长时间延时会保持CPU活跃状态，建议使用睡眠模式替代。

## 配置要求

使用Timer驱动前，需要确保以下配置正确：

```c
// 在配置文件中定义CPU频率（MHz）
#define CONFIG_CPU_FREQ_MHZ  50  // 示例：50MHz系统时钟
```

该配置直接影响延时计算的准确性，必须与实际硬件时钟频率保持一致。

## 版本信息

- **版本**: v1.0
- **最后更新**: 2025年10月
- **许可证**: Apache-2.0