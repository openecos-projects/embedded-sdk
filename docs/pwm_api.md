# PWM Driver API Documentation

## 概述

ECOS PWM驱动提供了对PWM（Pulse Width Modulation，脉宽调制）信号的完整控制接口。PWM是一种通过改变脉冲宽度来控制平均功率输出的技术，广泛应用于电机控制、LED亮度调节、音频信号生成等场景。该驱动支持4路独立的PWM输出通道。

## 头文件

```c
#include "pwm.h"
```

## 数据类型

### pwm_config_t

PWM配置结构体，用于初始化PWM控制器。

```c
typedef struct {
    uint32_t pscr;    ///< PWM预分频值，用于设置PWM基础时钟频率
    uint32_t cmp;     ///< PWM比较值，决定PWM周期长度
} pwm_config_t;
```

### pwm_channel_t

PWM通道枚举，定义可用的PWM输出通道。

```c
typedef enum {
    PWM_CH0 = 0,    ///< PWM通道0
    PWM_CH1 = 1,    ///< PWM通道1
    PWM_CH2 = 2,    ///< PWM通道2
    PWM_CH3 = 3,    ///< PWM通道3
} pwm_channel_t;
```

## 寄存器定义

PWM驱动提供了以下寄存器的直接访问：

```c
#define REG_CUST_PWM_CTRL  (*(volatile uint32_t*)0x03007000)  // PWM控制寄存器
#define REG_CUST_PWM_PSCR  (*(volatile uint32_t*)0x03007004)  // PWM预分频寄存器
#define REG_CUST_PWM_CNT   (*(volatile uint32_t*)0x03007008)  // PWM计数器寄存器
#define REG_CUST_PWM_CMP   (*(volatile uint32_t*)0x0300700c)  // PWM比较寄存器
#define REG_CUST_PWM_CR0   (*(volatile uint32_t*)0x03007010)  // PWM通道0比较寄存器
#define REG_CUST_PWM_CR1   (*(volatile uint32_t*)0x03007014)  // PWM通道1比较寄存器
#define REG_CUST_PWM_CR2   (*(volatile uint32_t*)0x03007018)  // PWM通道2比较寄存器
#define REG_CUST_PWM_CR3   (*(volatile uint32_t*)0x0300701c)  // PWM通道3比较寄存器
#define REG_CUST_PWM_STAT  (*(volatile uint32_t*)0x03007020)  // PWM状态寄存器
```

## API函数

### pwm_init()

**功能描述：** 初始化PWM控制器，配置预分频、比较值和启用PWM。

**函数原型：**
```c
void pwm_init(pwm_config_t* config);
```

**参数：**
- `config`: 指向pwm_config_t结构体的指针，包含预分频和比较值配置

**返回值：** 无

**工作原理：**
1. 设置预分频值，决定PWM基础时钟频率
2. 设置比较值，决定PWM周期长度
3. 启用PWM控制器

**PWM频率计算：**
```
PWM_FREQ = CPU_CLK / ((pscr + 1) * (cmp + 1))
```

**使用示例：**
```c
// 初始化PWM，生成1kHz的PWM信号
// 假设CPU时钟为72MHz
// PWM_FREQ = 72MHz / ((71 + 1) * (999 + 1)) = 1kHz
pwm_config_t pwm_conf = {
    .pscr = 71,   // 预分频值：72MHz / 72 = 1MHz
    .cmp = 999,   // 比较值：1MHz / 1000 = 1kHz
};
pwm_init(&pwm_conf);
```

### pwm_set_compare()

**功能描述：** 设置指定PWM通道的比较值，控制该通道的占空比。

**函数原型：**
```c
void pwm_set_compare(pwm_channel_t channel, uint32_t compare_value);
```

**参数：**
- `channel`: PWM通道选择（PWM_CH0到PWM_CH3）
- `compare_value`: 比较值，决定PWM占空比

**返回值：** 无

**占空比计算：**
```
Duty_Cycle = (compare_value / (cmp + 1)) * 100%
```

**使用示例：**
```c
// 设置PWM通道0的占空比为50%
// 如果cmp=999，则compare_value=500时占空比为50%
pwm_set_compare(PWM_CH0, 500);

// 设置PWM通道1的占空比为25%
pwm_set_compare(PWM_CH1, 250);

// 设置PWM通道2的占空比为75%
pwm_set_compare(PWM_CH2, 750);

// 设置PWM通道3的占空比为10%
pwm_set_compare(PWM_CH3, 100);
```

## 完整使用示例

### LED亮度控制示例

```c
#include "pwm.h"
#include "timer.h"
#include <stdio.h>

void led_brightness_control(void)
{
    // 初始化PWM，频率为1kHz
    pwm_config_t pwm_conf = {
        .pscr = 71,   // 72MHz / 72 = 1MHz
        .cmp = 999,   // 1MHz / 1000 = 1kHz
    };
    pwm_init(&pwm_conf);
    
    printf("LED brightness control demo\n");
    
    while (1) {
        // 渐亮效果
        printf("LED fading in...\n");
        for (uint32_t brightness = 0; brightness <= 1000; brightness += 10) {
            pwm_set_compare(PWM_CH0, brightness);
            delay_ms(20);  // 20ms延时，总共2秒渐亮
        }
        
        delay_ms(1000);  // 保持最亮1秒
        
        // 渐暗效果
        printf("LED fading out...\n");
        for (uint32_t brightness = 1000; brightness > 0; brightness -= 10) {
            pwm_set_compare(PWM_CH0, brightness);
            delay_ms(20);  // 20ms延时，总共2秒渐暗
        }
        
        delay_ms(1000);  // 保持熄灭1秒
    }
}
```

### 多通道RGB LED控制示例

```c
#include "pwm.h"
#include "timer.h"
#include <stdio.h>
#include <math.h>

// RGB颜色结构体
typedef struct {
    uint32_t red;
    uint32_t green;
    uint32_t blue;
} rgb_color_t;

void rgb_led_control(void)
{
    // 初始化PWM，频率为2kHz（更高频率减少闪烁）
    pwm_config_t pwm_conf = {
        .pscr = 35,   // 72MHz / 36 = 2MHz
        .cmp = 999,   // 2MHz / 1000 = 2kHz
    };
    pwm_init(&pwm_conf);
    
    printf("RGB LED color cycling demo\n");
    
    // 预定义颜色数组
    rgb_color_t colors[] = {
        {1000, 0, 0},      // 红色
        {1000, 500, 0},    // 橙色
        {1000, 1000, 0},   // 黄色
        {0, 1000, 0},      // 绿色
        {0, 1000, 1000},   // 青色
        {0, 0, 1000},      // 蓝色
        {500, 0, 1000},    // 紫色
        {1000, 0, 1000},   // 品红色
    };
    
    uint32_t color_count = sizeof(colors) / sizeof(colors[0]);
    
    while (1) {
        for (uint32_t i = 0; i < color_count; i++) {
            printf("Setting color %d: R=%d, G=%d, B=%d\n", 
                   i, colors[i].red, colors[i].green, colors[i].blue);
            
            // 设置RGB三个通道的PWM值
            pwm_set_compare(PWM_CH0, colors[i].red);    // 红色通道
            pwm_set_compare(PWM_CH1, colors[i].green);  // 绿色通道
            pwm_set_compare(PWM_CH2, colors[i].blue);   // 蓝色通道
            
            delay_ms(1000);  // 每种颜色显示1秒
        }
    }
}
```

### 电机速度控制示例

```c
#include "pwm.h"
#include "timer.h"
#include <stdio.h>

void motor_speed_control(void)
{
    // 初始化PWM，频率为20kHz（电机控制常用频率）
    pwm_config_t pwm_conf = {
        .pscr = 3,     // 72MHz / 4 = 18MHz
        .cmp = 899,    // 18MHz / 900 = 20kHz
    };
    pwm_init(&pwm_conf);
    
    printf("Motor speed control demo\n");
    
    // 电机速度等级（占空比百分比）
    uint32_t speed_levels[] = {0, 25, 50, 75, 100};
    uint32_t level_count = sizeof(speed_levels) / sizeof(speed_levels[0]);
    
    while (1) {
        for (uint32_t i = 0; i < level_count; i++) {
            uint32_t duty_cycle = speed_levels[i];
            uint32_t compare_value = (duty_cycle * 900) / 100;  // 转换为比较值
            
            printf("Setting motor speed to %d%% (compare=%d)\n", 
                   duty_cycle, compare_value);
            
            pwm_set_compare(PWM_CH0, compare_value);
            delay_ms(3000);  // 每个速度保持3秒
        }
    }
}
```

### 舵机控制示例

```c
#include "pwm.h"
#include "timer.h"
#include <stdio.h>

void servo_control(void)
{
    // 初始化PWM，频率为50Hz（舵机标准频率）
    pwm_config_t pwm_conf = {
        .pscr = 71,      // 72MHz / 72 = 1MHz
        .cmp = 19999,    // 1MHz / 20000 = 50Hz (20ms周期)
    };
    pwm_init(&pwm_conf);
    
    printf("Servo control demo\n");
    
    // 舵机角度对应的脉宽时间（微秒）
    // 0度: 1000us, 90度: 1500us, 180度: 2000us
    uint32_t servo_positions[] = {
        1000,  // 0度   (1000us / 20000us * 20000 = 1000)
        1250,  // 45度  (1250us / 20000us * 20000 = 1250)
        1500,  // 90度  (1500us / 20000us * 20000 = 1500)
        1750,  // 135度 (1750us / 20000us * 20000 = 1750)
        2000,  // 180度 (2000us / 20000us * 20000 = 2000)
    };
    
    uint32_t position_count = sizeof(servo_positions) / sizeof(servo_positions[0]);
    
    while (1) {
        for (uint32_t i = 0; i < position_count; i++) {
            printf("Moving servo to position %d (pulse width: %dus)\n", 
                   i * 45, servo_positions[i]);
            
            pwm_set_compare(PWM_CH0, servo_positions[i]);
            delay_ms(1000);  // 每个位置保持1秒
        }
    }
}
```

### 音频信号生成示例

```c
#include "pwm.h"
#include "timer.h"
#include <stdio.h>

// 音符频率定义（Hz）
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

typedef struct {
    uint32_t frequency;
    uint32_t duration_ms;
} note_t;

void audio_tone_generator(void)
{
    printf("Audio tone generator demo\n");
    
    // 简单的音阶
    note_t melody[] = {
        {NOTE_C4, 500},
        {NOTE_D4, 500},
        {NOTE_E4, 500},
        {NOTE_F4, 500},
        {NOTE_G4, 500},
        {NOTE_A4, 500},
        {NOTE_B4, 500},
        {NOTE_C5, 1000},
    };
    
    uint32_t note_count = sizeof(melody) / sizeof(melody[0]);
    
    while (1) {
        for (uint32_t i = 0; i < note_count; i++) {
            uint32_t frequency = melody[i].frequency;
            uint32_t duration = melody[i].duration_ms;
            
            if (frequency > 0) {
                // 计算PWM参数以生成指定频率
                // 假设CPU时钟72MHz，目标频率为frequency Hz
                uint32_t total_counts = 72000000 / frequency;
                uint32_t pscr = 0;
                uint32_t cmp = total_counts - 1;
                
                // 如果cmp太大，使用预分频
                while (cmp > 65535) {
                    pscr++;
                    cmp = (72000000 / ((pscr + 1) * frequency)) - 1;
                }
                
                printf("Playing note %dHz for %dms (pscr=%d, cmp=%d)\n", 
                       frequency, duration, pscr, cmp);
                
                // 重新初始化PWM以改变频率
                pwm_config_t pwm_conf = {
                    .pscr = pscr,
                    .cmp = cmp,
                };
                pwm_init(&pwm_conf);
                
                // 设置50%占空比产生方波音频信号
                pwm_set_compare(PWM_CH0, cmp / 2);
            } else {
                // 静音
                printf("Silence for %dms\n", duration);
                pwm_set_compare(PWM_CH0, 0);
            }
            
            delay_ms(duration);
        }
        
        delay_ms(2000);  // 播放完成后暂停2秒
    }
}
```

### 多通道同步控制示例

```c
#include "pwm.h"
#include "timer.h"
#include <stdio.h>

void multi_channel_sync_control(void)
{
    // 初始化PWM，频率为1kHz
    pwm_config_t pwm_conf = {
        .pscr = 71,   // 72MHz / 72 = 1MHz
        .cmp = 999,   // 1MHz / 1000 = 1kHz
    };
    pwm_init(&pwm_conf);
    
    printf("Multi-channel synchronized PWM demo\n");
    
    uint32_t step = 0;
    
    while (1) {
        // 通道0：正弦波模拟（使用查找表）
        uint32_t sin_value = (uint32_t)(500 + 400 * sin(step * 0.1));
        pwm_set_compare(PWM_CH0, sin_value);
        
        // 通道1：三角波
        uint32_t triangle_value;
        if ((step % 100) < 50) {
            triangle_value = (step % 50) * 20;  // 上升沿
        } else {
            triangle_value = 1000 - ((step % 50) * 20);  // 下降沿
        }
        pwm_set_compare(PWM_CH1, triangle_value);
        
        // 通道2：方波（50%占空比，周期性开关）
        uint32_t square_value = ((step % 50) < 25) ? 1000 : 0;
        pwm_set_compare(PWM_CH2, square_value);
        
        // 通道3：锯齿波
        uint32_t sawtooth_value = (step % 100) * 10;
        pwm_set_compare(PWM_CH3, sawtooth_value);
        
        printf("Step %d: CH0=%d, CH1=%d, CH2=%d, CH3=%d\n", 
               step, sin_value, triangle_value, square_value, sawtooth_value);
        
        step++;
        delay_ms(50);  // 50ms更新间隔
    }
}
```

## 频率和占空比配置

### 常用频率配置

```c
// 假设CPU时钟为72MHz

// 1kHz PWM（通用控制）
pwm_config_t config_1khz = { .pscr = 71, .cmp = 999 };

// 10kHz PWM（电机控制）
pwm_config_t config_10khz = { .pscr = 7, .cmp = 899 };

// 20kHz PWM（高频电机控制）
pwm_config_t config_20khz = { .pscr = 3, .cmp = 899 };

// 50Hz PWM（舵机控制）
pwm_config_t config_50hz = { .pscr = 71, .cmp = 19999 };

// 100Hz PWM（LED调光）
pwm_config_t config_100hz = { .pscr = 71, .cmp = 9999 };
```

### 占空比计算工具函数

```c
// 根据百分比计算比较值
uint32_t duty_to_compare(uint32_t duty_percent, uint32_t cmp_max)
{
    if (duty_percent > 100) duty_percent = 100;
    return (duty_percent * (cmp_max + 1)) / 100;
}

// 根据比较值计算占空比百分比
uint32_t compare_to_duty(uint32_t compare_value, uint32_t cmp_max)
{
    return (compare_value * 100) / (cmp_max + 1);
}

// 使用示例
void duty_cycle_example(void)
{
    pwm_config_t pwm_conf = { .pscr = 71, .cmp = 999 };
    pwm_init(&pwm_conf);
    
    // 设置25%占空比
    uint32_t compare_25 = duty_to_compare(25, 999);
    pwm_set_compare(PWM_CH0, compare_25);
    
    // 设置75%占空比
    uint32_t compare_75 = duty_to_compare(75, 999);
    pwm_set_compare(PWM_CH1, compare_75);
    
    printf("25%% duty cycle: compare = %d\n", compare_25);
    printf("75%% duty cycle: compare = %d\n", compare_75);
}
```

## 性能特性

### 频率范围

- **最高频率**: 约18MHz（pscr=0, cmp=3）
- **最低频率**: 约1Hz（pscr=65535, cmp=65535）
- **推荐范围**: 50Hz - 100kHz

### 分辨率

- **时间分辨率**: 取决于(pscr+1)的值
- **占空比分辨率**: 取决于(cmp+1)的值
- **最高分辨率**: 16位（65536级）

### 通道特性

- **通道数量**: 4路独立PWM输出
- **同步性**: 所有通道共享同一时钟源，天然同步
- **独立控制**: 每个通道可独立设置占空比

## 注意事项

1. **频率限制**：PWM频率受CPU时钟和寄存器位宽限制，过高或过低的频率可能无法实现。

2. **分辨率权衡**：高频率通常意味着低分辨率，需要根据应用需求平衡。

3. **硬件连接**：确保PWM输出引脚正确连接到目标设备。

4. **负载特性**：不同负载（LED、电机、舵机）对PWM频率和占空比有不同要求。

5. **电源管理**：PWM控制的设备功耗与占空比成正比。

6. **EMI考虑**：高频PWM可能产生电磁干扰，需要适当的滤波和屏蔽。

7. **精度限制**：由于整数运算，实际频率和占空比可能与理论值略有差异。

8. **启动顺序**：建议先初始化PWM，再设置各通道的比较值。

## 应用场景

### LED控制
- **亮度调节**: 通过改变占空比控制LED亮度
- **颜色混合**: RGB LED的红绿蓝三色独立控制
- **呼吸灯效果**: 周期性改变占空比实现渐亮渐暗

### 电机控制
- **直流电机调速**: 占空比控制电机转速
- **步进电机驱动**: 精确的脉冲序列控制
- **风扇调速**: PWM信号控制风扇转速

### 舵机控制
- **角度控制**: 1-2ms脉宽控制舵机角度
- **机械臂控制**: 多路舵机协调运动
- **云台控制**: 精确的位置控制

### 音频应用
- **蜂鸣器驱动**: 不同频率产生不同音调
- **简单音乐播放**: 音符序列播放
- **报警信号**: 特定频率的警告音

### 加热控制
- **温度调节**: 占空比控制加热功率
- **PID控制**: 闭环温度控制系统
- **功率限制**: 防止过热保护

## 版本信息

- **版本**: v1.0
- **最后更新**: 2025年10月
- **许可证**: Apache-2.0