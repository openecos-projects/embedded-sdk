# ECOS SDK 2.0 HAL 层设计

## 📊 当前架构分析

### 现有结构问题
1. **紧耦合设计**：组件（components）直接依赖板卡头文件（board.h）
   - GPIO 组件直接包含 `board.h` 和 `generated/autoconf.h`
   - 寄存器地址硬编码在板卡头文件中
   - 组件代码中直接使用 `REG_GPIO_0_DDR` 等宏

2. **缺乏抽象层**：
   - 没有硬件抽象层，组件与具体硬件实现紧密绑定
   - 不同板卡的差异通过条件编译处理（`#if CONFIG_GPIO_IP_ID == 0`）
   - 扩展新板卡需要修改组件源码

3. **可移植性差**：
   - 组件无法在不同硬件平台上复用
   - 添加新外设需要大量重复代码
   - 缺乏统一的驱动接口标准

### 当前代码结构
```
components/
├── gpio/                    # 外设组件
│   ├── include/gpio.h       # API 接口
│   ├── include/gpio_type.h  # 数据类型
│   └── src/gpio.c           # 实现（直接调用寄存器）
board/
├── StarrySkyC1/             # 板卡配置
│   └── board.h              # 寄存器地址定义
├── StarrySkyC2/
└── StarrySkyL3/
```

## 🎯 HAL 层设计目标

### 核心原则
1. **解耦合**：组件 ↔ HAL ↔ 板卡驱动
2. **标准化**：统一的驱动接口规范
3. **可扩展**：易于添加新外设和新板卡
4. **可移植**：组件代码无需修改即可适配不同硬件

### 架构分层
```
Application Layer
       ↓
Component Layer (API)      # 保持现有 API 不变
       ↓
HAL Layer (Interface)      # 新增：硬件抽象接口
       ↓
Driver Layer (Implementation) # 新增：具体硬件驱动
       ↓
Board Layer                # 现有：板卡特定配置
```

## 🏗️ HAL 层详细设计

### 1. 目录结构调整
```
ecos-sdk/
├── components/            # 保持现有 API 接口
├── hal/                   # 新增：HAL 接口层
│   ├── include/           # HAL 接口头文件
│   │   ├── hal_gpio.h
│   │   ├── hal_uart.h
│   │   └── hal_common.h
│   └── src/               # HAL 通用实现（可选）
├── drivers/               # 新增：驱动实现层
│   ├── gpio/              # GPIO 驱动
│   │   ├── starrysky_c1/  # 具体板卡驱动
│   │   ├── starrysky_c2/
│   │   └── generic/       # 通用驱动
│   └── uart/              # UART 驱动
└── board/                 # 现有板卡配置（重构）
```

### 2. HAL 接口定义
```c
// hal/include/hal_gpio.h
#ifndef HAL_GPIO_H__
#define HAL_GPIO_H__

#include <stdint.h>
#include "hal_common.h"

// HAL 层统一数据类型
typedef enum {
    HAL_GPIO_MODE_INPUT = 0,
    HAL_GPIO_MODE_OUTPUT = 1,
} hal_gpio_mode_t;

typedef struct {
    uint64_t pin_bit_mask;
    hal_gpio_mode_t mode;
} hal_gpio_config_t;

// HAL 层统一接口
typedef struct {
    void (*config)(const hal_gpio_config_t* config);
    void (*set_direction)(uint32_t pin, hal_gpio_mode_t mode);
    void (*set_level)(uint32_t pin, uint32_t level);
    uint32_t (*get_level)(uint32_t pin);
    void (*set_function)(uint32_t pin, uint32_t func);
} hal_gpio_driver_t;

// 驱动注册函数
void hal_gpio_register_driver(const hal_gpio_driver_t* driver);

#endif
```

### 3. 驱动实现示例
```c
// drivers/gpio/starrysky_c1/gpio_driver.c
#include "hal_gpio.h"
#include "board.h"  // 仅驱动层包含板卡头文件

static void gpio_config_impl(const hal_gpio_config_t* config) {
    // 实现具体的寄存器操作
    // 使用 board.h 中定义的寄存器地址
}

static hal_gpio_driver_t starrysky_c1_gpio_driver = {
    .config = gpio_config_impl,
    .set_direction = gpio_set_direction_impl,
    .set_level = gpio_set_level_impl,
    .get_level = gpio_get_level_impl,
    .set_function = gpio_set_function_impl,
};

// 初始化时注册驱动
void starrysky_c1_gpio_init(void) {
    hal_gpio_register_driver(&starrysky_c1_gpio_driver);
}
```

### 4. 组件层改造
```c
// components/gpio/src/gpio.c (改造后)
#include "gpio.h"
#include "hal_gpio.h"  // 改为包含 HAL 接口

void gpio_config(const gpio_config_t* config) {
    // 转换数据类型
    hal_gpio_config_t hal_config = {
        .pin_bit_mask = config->pin_bit_mask,
        .mode = (config->mode == GPIO_MODE_INPUT) ? 
                HAL_GPIO_MODE_INPUT : HAL_GPIO_MODE_OUTPUT
    };
    
    // 调用 HAL 接口
    hal_gpio_driver.config(&hal_config);
}
```

### 5. 板卡初始化
```c
// board/StarrySkyC1/init.c
#include "drivers/gpio/starrysky_c1/gpio_driver.h"
#include "drivers/uart/starrysky_c1/uart_driver.h"

void board_init(void) {
    starrysky_c1_gpio_init();
    starrysky_c1_uart_init();
    // ... 其他外设初始化
}
```

## 🔧 实施步骤

### Phase 1: 基础框架搭建
- [ ] 创建 `hal/` 和 `drivers/` 目录结构
- [ ] 定义 HAL 通用接口规范 (`hal_common.h`)
- [ ] 实现 HAL 注册和管理机制

### Phase 2: 核心外设迁移
- [ ] GPIO HAL 接口和驱动实现
- [ ] UART HAL 接口和驱动实现  
- [ ] Timer HAL 接口和驱动实现
- [ ] 更新对应组件层代码

### Phase 3: 其他外设迁移
- [ ] I2C、SPI、PWM 等外设 HAL 实现
- [ ] 完善驱动注册和初始化流程
- [ ] 更新构建系统和配置文件

### Phase 4: 测试和验证
- [ ] 单元测试：HAL 接口功能验证
- [ ] 集成测试：现有模板项目兼容性
- [ ] 性能测试：确保无性能损失
- [ ] 文档更新：HAL 开发指南

## 📈 预期收益

### 技术优势
- **可维护性**：组件与硬件解耦，修改硬件不影响上层
- **可扩展性**：添加新板卡只需实现驱动，无需修改组件
- **可移植性**：同一组件可在不同硬件平台复用
- **标准化**：统一的驱动开发规范

### 开发体验
- **降低门槛**：新开发者只需关注 HAL 接口
- **提高效率**：驱动开发模板化，减少重复工作
- **便于测试**：可以mock HAL 层进行组件测试

## ⚠️ 注意事项

1. **API 兼容性**：保持现有组件 API 不变，确保向后兼容
2. **性能影响**：HAL 层增加一层函数调用，需优化避免性能损失
3. **内存占用**：驱动结构体和函数指针会增加少量内存占用
4. **构建复杂度**：需要更新构建系统以支持新的目录结构

---
最后更新: 2026-03-13
设计者: Timmo Li