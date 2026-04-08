# SDK配置

## Menuconfig分类
### Target Hardware Configuration
存放与板卡型号有关的只读信息

### System Configuration
存放与板卡型号有关的可配置信息

### Peripheral Drivers
存放与外设相关的可配置信息

### Build Configuration
存放与编译链接相关的可配置信息

### Firmware Configuration
存放与固件最终构建相关的可配置信息

## 新版卡的适配方式
Menuconfig引用链为：Makefile -> build_conf.mk -> auto.conf(由script/config.mk、kconfig/Kconfig、board/板卡型号/*.kconfig共同生成)
1. 创建board/板卡型号/*.kconfig，并编写与板卡型号有关的只读信息/可配置信息
2. 配置script/config.mk，将板卡型号添加进CATEGORY_LIST
3. 配置Makefile，先包含auto.conf，再包含build_conf.mk
4. 创建build_conf.mk，使用生成的配置信息，来配置项目的编译链接
在适配新板卡时，推荐使用已有板卡的配置文件作为参考，并在此基础上进行修改

## 新驱动的适配方式
驱动相关的源文件：board/板卡型号/driver/驱动/*.c
驱动相关的头文件：hal/驱动/*.h
驱动相关的kconfig：board/板卡型号/driver.kconfig
关联驱动并配置的文件：board/板卡型号/build_conf.mk
1. 添加驱动的源文件、头文件
2. 在kconfig中添加该驱动
3. 由于build_conf.mk可以自动识别驱动并配置，因此无需修改
