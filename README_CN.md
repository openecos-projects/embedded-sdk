# 简介
本仓库包含了StarrySky_Pi的使用示例及外设IP的驱动库

目前已支持以下驱动：
- I2C 读写
- 定时器配置，延时函数
- SPI写

规划中：
- SPI读写
- PWM
- GPIO
- ST7735 屏幕控制器

# 使用说明
由于本仓库只提供相关库文件，AM部分仍需大家自行完成

1. 将klib相关文件加入到AM的klib中
2. 配置AM_HOME环境变量
3. 在ysyx_env/prog/src目录下运行make脚本即可（使用make help获取编译格式） 
