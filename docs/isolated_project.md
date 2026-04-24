# 分离式工程

## 功能
生成分离式工程（Isolated Project）时，将会按照板卡类型收集 SDK 中所有可用资源，使得无需 SDK 环境也可以编译项目。 但 riscv-toolchain 仍然是不可缺少的组件。

## 项目结构
ECOS_Isolated_Project/
├── README.md          # 项目说明文档
├── Makefile           # 构建配置
├── script/            # 构建工具
├── configs/           # 构建的编译清单
├── build/             # 生成的SoC固件
├── Library/           # 工作区: 包含与Core相关的文件、SDK提供的标准库文件
├── Startup/           # 工作区: 包含与系统链接与链接相关的文件
├── User/              # 工作区: 包含自定义的程序入口
├── System/            # 工作区: 包含SDK提供的组件、自定义的片上资源驱动
└── Hardware/          # 工作区: 包含自定义的片外资源驱动

工作区中，除了Library、Startup、User、System是正常运行所必须的文件夹以外，其他文件夹均可以任意自定义

## 创建一个新的分离式工程

```shell
ecos init_project <项目名称> -isolated -name <项目新名称> -target <板卡类型>
make menuconfig
make
# 脱离SDK环境后
make
```

1. 目前仅适配C2板卡
2. 脱离SDK环境后，如果需要使用menuconfig，必须重新接入SDK环境
2. 调整menuconfig中的Peripheral Drivers与External Devices是无效的
  - 在分离式项目中，调整它只会影响到对应宏的生成，不会影响到头文件包含
  - 在分离式项目中，它们均默认加入到CFLAGS的头文件搜索路径中
  - 您可以使用configs/generated/autoconf.h、configs/config/auto.conf来手动调整头文件包含

## 新版卡的适配方式
注意，板卡资源仅包含板卡相关的文件、文件夹（SDK/board下的资源）

1. 在ecos-set_board_isolated.sh的set_board函数中，编写“移动新板卡的相关文件”的逻辑
2. 执行./install.sh更新SDK

## 新SDK资源的适配方式
注意，SDK资源仅包含板卡不相关的文件、文件夹（SDK/board以外的资源）

1. 在ecos-init_project.sh的init_project_isolated函数中，编写“移动SDK新资源”的逻辑
2. 执行./install.sh更新SDK