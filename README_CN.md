# ECOS 嵌入式SDK
## 快速开始
1. 确认riscv交叉编译工具链
    1. 确认riscv交叉编译工具链已安装并添加到PATH环境变量中
    2. 确认riscv交叉编译工具链类型与Makefile中配置的类型一致 ```utils/abstract-machine/scripts/riscv-mycpu.mk```

2. 确认AM_HOME环境变量已设置为'''utils/abstract-machine'''的路径
3. 导航到src目录: `cd prog/src`
4. 运行make脚本: `make`
    - 使用`make help`获取编译格式

## 支持的驱动
- I2C 读写
- 定时器配置，延时函数
- SPI写

## 计划中的驱动
- SPI 读写
- PWM
- GPIO
- ST7735 屏幕控制器
