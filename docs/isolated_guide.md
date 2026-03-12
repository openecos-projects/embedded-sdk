# 分离式项目

## 项目概述
生成分离式项目将会按照板卡类型，收集 SDK 中所有可用资源，使得无需 SDK 即可编译项目。
但 riscv-toolchain 仍然是不可缺少的组件。

## 项目结构
```
ECOS_Isolated_Project/
├── README.md          # 项目说明文档
├── Makefile           # 构建配置
├── script/            # 构建工具
├── configs/           # 构建的编译清单
├── build/             # 生成的SoC固件
├── Library/           # 工作区: 包含与Core相关的文件、SDK提供的标准库文件
├── Startup/           # 工作区: 包含与系统链接与链接相关的文件
├── User/              # 工作区: 包含自定义的程序入口
├── System/            # 工作区: 包含自定义的片上资源驱动
└── Hardware/          # 工作区: 包含自定义的片外资源驱动
```

工作区中，除了Library、Startup、User是正常运行所必须的文件夹以外，其他文件夹可以任意自定义