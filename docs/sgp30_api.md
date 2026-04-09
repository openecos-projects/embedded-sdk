# SGP30 Air Quality Sensor API Documentation

## 概述

SGP30是Sensirion公司生产的多像素气体传感器，能够测量室内空气质量。该传感器可以检测多种挥发性有机化合物(VOCs)和氢气，并输出CO2当量和TVOC(总挥发性有机化合物)浓度值。

SGP30驱动提供了完整的传感器控制接口，包括设备信息读取、空气质量测量、基线校准和湿度补偿等功能。

## 特性

- **CO2当量测量**: 400-60000 ppm范围
- **TVOC测量**: 0-60000 ppb范围  
- **基线校准**: 支持基线值保存和恢复
- **湿度补偿**: 提高测量精度
- **自测试功能**: 验证传感器工作状态
- **CRC校验**: 确保数据传输完整性

## 头文件

```c
#include "sgp30.h"
```

## 数据类型

### sgp30_air_quality_t

空气质量测量数据结构体。

```c
typedef struct {
    uint16_t co2_eq_ppm;    ///< CO2当量浓度 (ppm)
    uint16_t tvoc_ppb;      ///< TVOC浓度 (ppb)
} sgp30_air_quality_t;
```

**字段说明:**
- `co2_eq_ppm`: CO2当量浓度，单位为ppm (parts per million)
- `tvoc_ppb`: 总挥发性有机化合物浓度，单位为ppb (parts per billion)

### sgp30_baseline_t

基线校准数据结构体。

```c
typedef struct {
    uint16_t co2_eq_baseline;    ///< CO2当量基线值
    uint16_t tvoc_baseline;      ///< TVOC基线值
} sgp30_baseline_t;
```

**字段说明:**
- `co2_eq_baseline`: CO2当量的基线校准值
- `tvoc_baseline`: TVOC的基线校准值

## 常量定义

### I2C地址
```c
#define SGP30_I2C_ADDR (0x58)    ///< SGP30的7位I2C地址
```

### 命令定义
```c
#define SGP30_CMD_GET_SERIAL_ID        (0x3682)    ///< 获取序列号命令
#define SGP30_CMD_GET_FEATURE_SET      (0x202F)    ///< 获取特征集命令
#define SGP30_CMD_INIT_AIR_QUALITY     (0x2003)    ///< 初始化空气质量测量命令
#define SGP30_CMD_MEASURE_AIR_QUALITY  (0x2008)    ///< 测量空气质量命令
#define SGP30_CMD_GET_BASELINE         (0x2015)    ///< 获取基线命令
#define SGP30_CMD_SET_BASELINE         (0x201E)    ///< 设置基线命令
#define SGP30_CMD_SET_HUMIDITY         (0x2061)    ///< 设置湿度补偿命令
#define SGP30_CMD_MEASURE_TEST         (0x2032)    ///< 自测试命令
```

## API函数

### sgp30_read_serial_id

读取SGP30传感器的48位序列号。

```c
bool sgp30_read_serial_id(uint64_t *serial_id);
```

**参数:**
- `serial_id`: 指向存储序列号的64位整数指针

**返回值:**
- `true`: 读取成功
- `false`: 读取失败或CRC校验错误

**示例:**
```c
uint64_t serial_id;
if (sgp30_read_serial_id(&serial_id)) {
    printf("SGP30 Serial ID: 0x%llX\n", serial_id);
} else {
    printf("Failed to read serial ID\n");
}
```

### sgp30_read_feature_set

读取SGP30传感器的特征集版本。

```c
bool sgp30_read_feature_set(uint16_t *feature_set);
```

**参数:**
- `feature_set`: 指向存储特征集版本的16位整数指针

**返回值:**
- `true`: 读取成功
- `false`: 读取失败或CRC校验错误

**示例:**
```c
uint16_t feature_set;
if (sgp30_read_feature_set(&feature_set)) {
    printf("SGP30 Feature Set: 0x%04X\n", feature_set);
}
```

### sgp30_init_air_quality

初始化SGP30的空气质量测量功能。

```c
bool sgp30_init_air_quality(void);
```

**返回值:**
- `true`: 初始化成功
- `false`: 初始化失败

**说明:**
- 必须在进行空气质量测量前调用此函数
- 初始化后需要等待15秒才能获得稳定的测量结果
- 在此期间，传感器会返回固定的初始值

**示例:**
```c
if (sgp30_init_air_quality()) {
    printf("SGP30 air quality measurement initialized\n");
    // 等待15秒获得稳定读数
    delay_ms(15000);
}
```

### sgp30_measure_air_quality

测量当前的空气质量数据。

```c
bool sgp30_measure_air_quality(sgp30_air_quality_t *air_quality);
```

**参数:**
- `air_quality`: 指向存储测量结果的结构体指针

**返回值:**
- `true`: 测量成功
- `false`: 测量失败或CRC校验错误

**说明:**
- 测量间隔应为1秒
- 初始化后的前15秒内，读数可能不稳定
- CO2当量范围: 400-60000 ppm
- TVOC范围: 0-60000 ppb

**示例:**
```c
sgp30_air_quality_t air_quality;
if (sgp30_measure_air_quality(&air_quality)) {
    printf("CO2eq: %d ppm, TVOC: %d ppb\n", 
           air_quality.co2_eq_ppm, air_quality.tvoc_ppb);
}
```

### sgp30_get_baseline

获取当前的基线校准值。

```c
bool sgp30_get_baseline(sgp30_baseline_t *baseline);
```

**参数:**
- `baseline`: 指向存储基线值的结构体指针

**返回值:**
- `true`: 获取成功
- `false`: 获取失败或CRC校验错误

**说明:**
- 基线值用于补偿传感器的长期漂移
- 建议每小时保存一次基线值
- 基线值可用于下次启动时快速校准

**示例:**
```c
sgp30_baseline_t baseline;
if (sgp30_get_baseline(&baseline)) {
    printf("CO2eq baseline: 0x%04X, TVOC baseline: 0x%04X\n",
           baseline.co2_eq_baseline, baseline.tvoc_baseline);
    // 保存基线值到非易失性存储器
}
```

### sgp30_set_baseline

设置基线校准值。

```c
bool sgp30_set_baseline(const sgp30_baseline_t *baseline);
```

**参数:**
- `baseline`: 指向包含基线值的结构体指针

**返回值:**
- `true`: 设置成功
- `false`: 设置失败

**说明:**
- 用于恢复之前保存的基线值
- 应在初始化后立即调用
- 可以显著减少传感器的稳定时间

**示例:**
```c
sgp30_baseline_t saved_baseline = {0x8973, 0x8AAE}; // 从存储器读取
if (sgp30_set_baseline(&saved_baseline)) {
    printf("Baseline restored successfully\n");
}
```

### sgp30_set_humidity

设置湿度补偿值以提高测量精度。

```c
bool sgp30_set_humidity(uint16_t humidity);
```

**参数:**
- `humidity`: 湿度补偿值 (相对湿度百分比 × 256)

**返回值:**
- `true`: 设置成功
- `false`: 设置失败

**说明:**
- 湿度补偿可以提高TVOC测量的准确性
- 输入值 = 相对湿度(%) × 256
- 例如: 50% RH = 50 × 256 = 12800

**示例:**
```c
// 设置50%相对湿度补偿
uint16_t humidity_50_percent = 50 * 256;
if (sgp30_set_humidity(humidity_50_percent)) {
    printf("Humidity compensation set to 50%% RH\n");
}
```

### sgp30_measure_test

执行传感器自测试。

```c
bool sgp30_measure_test(uint16_t *test_result);
```

**参数:**
- `test_result`: 指向存储测试结果的16位整数指针

**返回值:**
- `true`: 测试执行成功
- `false`: 测试失败或CRC校验错误

**说明:**
- 自测试需要220ms完成
- 正常情况下返回值应为0xD400
- 可用于验证传感器是否正常工作

**示例:**
```c
uint16_t test_result;
if (sgp30_measure_test(&test_result)) {
    if (test_result == 0xD400) {
        printf("SGP30 self-test PASSED\n");
    } else {
        printf("SGP30 self-test FAILED (0x%04X)\n", test_result);
    }
}
```

## 使用流程

### 基本使用流程

1. **初始化I2C**: 确保I2C总线已正确初始化
2. **读取设备信息**: 验证传感器连接和工作状态
3. **执行自测试**: 确认传感器功能正常
4. **初始化空气质量测量**: 启动测量功能
5. **设置湿度补偿**: (可选) 提高测量精度
6. **连续测量**: 每秒读取一次空气质量数据
7. **基线管理**: 定期保存和恢复基线值

### 完整示例

```c
#include "sgp30.h"
#include "i2c.h"

void sgp30_demo(void) {
    // 1. 初始化I2C
    i2c_config_t i2c_config = {.pscr = 99};
    i2c_init(&i2c_config);
    
    // 2. 读取设备信息
    uint64_t serial_id;
    uint16_t feature_set;
    
    if (!sgp30_read_serial_id(&serial_id)) {
        printf("Failed to read SGP30 serial ID\n");
        return;
    }
    printf("SGP30 Serial ID: 0x%llX\n", serial_id);
    
    if (!sgp30_read_feature_set(&feature_set)) {
        printf("Failed to read SGP30 feature set\n");
        return;
    }
    printf("SGP30 Feature Set: 0x%04X\n", feature_set);
    
    // 3. 执行自测试
    uint16_t test_result;
    if (sgp30_measure_test(&test_result)) {
        printf("Self-test result: 0x%04X %s\n", 
               test_result, (test_result == 0xD400) ? "PASS" : "FAIL");
    }
    
    // 4. 初始化空气质量测量
    if (!sgp30_init_air_quality()) {
        printf("Failed to initialize air quality measurement\n");
        return;
    }
    printf("Air quality measurement initialized\n");
    
    // 5. 设置湿度补偿 (50% RH)
    sgp30_set_humidity(50 * 256);
    
    // 6. 恢复基线值 (如果有保存的值)
    sgp30_baseline_t saved_baseline = {0x8973, 0x8AAE}; // 示例值
    sgp30_set_baseline(&saved_baseline);
    
    // 7. 连续测量
    printf("Starting continuous measurements...\n");
    for (int i = 0; i < 60; i++) {  // 测量60秒
        sgp30_air_quality_t air_quality;
        
        if (sgp30_measure_air_quality(&air_quality)) {
            printf("Measurement %d: CO2eq=%d ppm, TVOC=%d ppb\n", 
                   i+1, air_quality.co2_eq_ppm, air_quality.tvoc_ppb);
        } else {
            printf("Measurement %d: Failed\n", i+1);
        }
        
        delay_ms(1000);  // 等待1秒
        
        // 每10次测量保存一次基线
        if ((i + 1) % 10 == 0) {
            sgp30_baseline_t current_baseline;
            if (sgp30_get_baseline(&current_baseline)) {
                printf("Current baseline - CO2eq: 0x%04X, TVOC: 0x%04X\n",
                       current_baseline.co2_eq_baseline, 
                       current_baseline.tvoc_baseline);
                // 这里可以将基线值保存到非易失性存储器
            }
        }
    }
}
```

## 注意事项

### 测量精度
- SGP30需要15秒的稳定时间才能提供准确的测量结果
- 建议每秒进行一次测量以保持传感器的最佳性能
- 湿度补偿可以显著提高TVOC测量的准确性

### 基线管理
- 基线值会随时间漂移，建议每小时保存一次
- 保存的基线值可以在下次启动时恢复，减少稳定时间
- 基线值应保存在非易失性存储器中

### 错误处理
- 所有函数都包含CRC校验，确保数据完整性
- I2C通信错误会通过返回值false指示
- 建议在关键应用中添加重试机制

### 环境要求
- 工作温度: -40°C 到 +85°C
- 工作湿度: 0% 到 95% RH (无凝露)
- 电源电压: 1.62V 到 3.6V

## 故障排除

### 常见问题

1. **读取序列号失败**
   - 检查I2C连接和地址设置
   - 确认传感器电源供应正常
   - 检查I2C上拉电阻

2. **测量值异常**
   - 确认已调用初始化函数
   - 等待足够的稳定时间(15秒)
   - 检查环境条件是否在规格范围内

3. **自测试失败**
   - 传感器可能损坏
   - 检查电源电压是否稳定
   - 尝试重新上电

4. **基线值异常**
   - 确认传感器已运行足够时间
   - 检查保存的基线值是否有效
   - 重新进行基线校准

## 版本信息

- **驱动版本**: 1.0.0
- **支持的SGP30版本**: 所有版本
- **最后更新**: 2025年10月

## 相关文档

- [I2C API文档](i2c_api.md)
- [开发指南](development_guide.md)
- [SGP30数据手册](https://www.sensirion.com/sgp30/)