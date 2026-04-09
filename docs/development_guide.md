# retroSoC 开发入门指南

## 项目概述

retroSoC 是一个基于 RISC-V 架构的定制化 ASIC 项目。该项目提供了完整的嵌入式系统开发环境，包括硬件抽象层、外设驱动和开发工具链。

## 技术栈

### 硬件架构
- **处理器核心**: PicoRV32
- **指令集架构**: RISC-V rv32imac
- **工作频率**: 72MHz
- **目标架构**: riscv32-unknown-elf

### 内存系统
- **片上 SRAM**: 128KB @ 0x00000000-0x0001FFFF
- **外部 PSRAM**: 8MB @ 0x04000000-0x047FFFFF (1x8MB)
- **SPI Flash**: 16MB @ 0x30000000-0x30FFFFFF

### 外设模块
项目包含丰富的外设驱动模块：

#### 通信接口
- **UART**: 标准串口通信 (115200 8N1)
- **UART(HP)**: 高性能串口
- **SPI**: 标准 SPI 接口
- **HK SPI**: 专用 SPI 控制器
- **I2C**: I2C 总线接口
- **PS2**: PS2 键盘/鼠标接口

#### 存储与控制
- **GPIO**: 16路通用输入输出
- **PWM**: 4路脉宽调制输出
- **Timer**: 2路定时器 (TIM0, TIM1)
- **Flash**: SPI Flash 控制器
- **PSRAM**: 外部 PSRAM 控制器

#### 显示与其他
- **LCD**: LCD 显示控制器 (128x128)
- **RNG**: 随机数生成器
- **ArchInfo**: 系统架构信息

## 开发环境配置

### 目录结构
```
ECOS_env/
├── README.md           # 项目说明文档
├── doc/               # 文档目录
├── HW/                # 硬件设计文件
│   ├── Retro0_card_SCH.pdf
│   └── Retro0_card_v0.brd
├── src/               # 源代码目录
│   ├── Makefile       # 构建配置
│   ├── firmware.c     # 主程序
│   ├── start.s        # 启动汇编代码
│   ├── sections.lds   # 链接脚本
│   ├── tiny*.c/h      # 外设驱动模块
│   └── libgcc/        # GCC 运行时库
└── tools/             # 开发工具链
    └── riscv/         # RISC-V 交叉编译工具链
```

### 工具链配置

1. **下载工具链**
   - 下载工具链
   - 解压到 `tools/riscv/` 目录

2. **配置环境变量**
   ```bash
   export PATH=$PATH:/home/timmo/ECOS_env/tools/riscv/bin
   ```

3. **验证安装**
   ```bash
   riscv32-unknown-elf-gcc --version
   ```

## 编译构建

### 编译配置
项目使用 Makefile 进行构建管理，主要编译参数：

```makefile
CROSS=riscv32-unknown-elf-
CFLAGS := -mabi=ilp32 \
          -march=rv32im \
          -Wl,-Bstatic,-T,retrosoc_sections.lds,--strip-debug \
          -ffreestanding \
          -nostdlib
```

### 构建流程
1. **进入源码目录**
   ```bash
   cd src/
   ```

2. **执行编译**
   ```bash
   make retrosoc_fw.elf
   ```

3. **生成的文件**
   - `retrosoc_fw.elf`: ELF 可执行文件
   - `retrosoc_fw.hex`: Verilog 十六进制格式
   - `retrosoc_fw.bin`: 二进制固件文件
   - `retrosoc_fw.txt`: 反汇编文本文件

### 清理构建
```bash
make clean
```

## 内存布局

### 链接脚本配置
```
MEMORY {
    FLASH (rx)  : ORIGIN = 0x30000000, LENGTH = 16MB  # SPI Flash
    RAM (xrw)   : ORIGIN = 0x04000000, LENGTH = 8MB   # PSRAM
}
```

### 内存映射
- **代码段 (.text)**: 存储在 Flash 中
- **数据段 (.data)**: 运行时复制到 RAM
- **BSS 段 (.bss)**: 零初始化数据段
- **堆栈**: 动态分配区域

## 外设寄存器映射

### 基础外设 (0x03000000 基址)
- **GPIO**: 0x03000000 (数据/使能/上拉/下拉)
- **UART**: 0x03000010 (时钟分频/数据)
- **SPI**: 0x03000018 (配置/使能/PLL)
- **Timer**: 0x03001000, 0x03002000

### 定制外设 (0x03003000+ 基址)
- **UART(HP)**: 0x03003000
- **PWM**: 0x03004000
- **PS2**: 0x03005000
- **I2C**: 0x03006000
- **QSPI**: 0x03007000
- **PSRAM**: 0x03008000

## 开发指南

### 添加新的外设驱动

1. **创建头文件** (`tinyxxx.h`)
   ```c
   #ifndef TINYXXX_H__
   #define TINYXXX_H__
   
   // 寄存器定义
   #define reg_xxx_ctrl (*(volatile uint32_t*)0x03009000)
   
   // 函数声明
   void xxx_init();
   void xxx_test();
   
   #endif
   ```

2. **实现源文件** (`tinyxxx.c`)
   ```c
   #include "tinyxxx.h"
   #include "tinylib.h"
   
   void xxx_init() {
       // 初始化代码
   }
   
   void xxx_test() {
       // 测试代码
   }
   ```

3. **更新构建配置**
   - 在 `Makefile` 的 `SRC_PATH` 中添加新文件
   - 在 `tinylib.h` 中包含新头文件

### 调试技巧

1. **使用 printf 调试**
   ```c
   printf("Debug: value = %d\n", value);
   ```

2. **查看反汇编**
   ```bash
   riscv32-unknown-elf-objdump -d retrosoc_fw.elf > debug.txt
   ```

3. **内存转储**
   ```bash
   riscv32-unknown-elf-objcopy -O binary retrosoc_fw.elf firmware.bin
   hexdump -C firmware.bin | head -20
   ```

## 常见问题

### Q: 编译时提示找不到工具链？
A: 检查 PATH 环境变量是否正确设置，确保 `riscv32-unknown-elf-gcc` 可执行。

### Q: 如何修改系统频率？
A: 修改 `firmware.h` 中的 `CPU_FREQ` 宏定义，重新编译即可。

### Q: 如何添加新的内存区域？
A: 修改 `sections.lds` 链接脚本，添加新的 MEMORY 定义和对应的 SECTIONS。

### Q: 外设寄存器访问异常？
A: 检查寄存器地址是否正确，确保在访问前已正确初始化外设时钟和使能。

## 参考资料

- [RISC-V 指令集手册](https://riscv.org/specifications/)
- [PicoRV32 处理器文档](https://github.com/YosysHQ/picorv32)
- [GNU 工具链文档](https://gcc.gnu.org/onlinedocs/)
- [项目 GitHub 仓库](https://github.com/retroSoC/retroSoC)

---

*本文档由 AI 助手基于项目源码自动生成，如有疑问请参考源码或联系项目维护者。*