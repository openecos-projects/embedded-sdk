# SDK配置

## Menuconfig分类
### Target Hardware Configuration
存放与板卡型号有关的只读信息

### System Configuration
存放除了外设、编译链接、固件最终构建以外的可配置信息

### Peripheral Drivers
存放与外设相关的可配置信息

### Build Configuration
存放与编译链接相关的可配置信息

### Firmware Configuration
存放与固件最终构建相关的可配置信息

## 新版卡的Menuconfig适配方式
Menuconfig引用链为：Makefile -> build_conf.mk -> auto.conf(由script/config.mk、kconfig/Kconfig、board/板卡型号/Kconfig共同生成)
1. 创建board/Kconfig，并编写与板卡型号有关的只读信息（显示在Target Hardware Configuration中）
2. 配置script/config.mk，将板卡型号添加进CATEGORY_LIST
3. 配置Makefile，先包含auto.conf，再包含build_conf.mk
4. 创建build_conf.mk，使用生成的配置信息，来配置项目的编译链接
在适配新板卡时，推荐使用已有板卡的配置文件作为参考，并在此基础上进行修改


