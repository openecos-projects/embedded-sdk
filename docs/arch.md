# 架构设计

## 核心功能
- 快速创建模板工程
- 编译工程
- 烧录及监视目标设备

## 指令设计
“ecos <command>”:
    - <init_project> [-t <target_name>] [-n <project_name>]: 初始化项目
        - -t <target_name>: 目标板卡
        - -n <project_name>: 项目名称
    - <set_target> <target>: 设置当前工程的目标板卡
        - <target>: 目标板卡名称
    - <flash>: 烧录当前工程编译结果到目标板卡【仅支持linux平台】
    - <monitor>: 监视目标板卡运行状态【仅支持linux平台】

## SDK结构设计
├── bin # ecos 可执行文件目录
├── board # 支持的目标板卡目录
│   ├── StarrySkyC2 # 板卡目录
│   │   ├── driver # 驱动目录
│   │   └── ...... # 其他相关文件
│   └── ...... # 其他板卡目录
│
├── hal # 高级抽象层目录
│   ├── gpio # GPIO抽象层目录
│   │   ├── gpio.h # GPIO头文件
│   │   └── ...... # 其他GPIO相关文件
│   └── ...... # 其他HAL相关目录
│
├── device # 基于HAL的设备驱动
│   ├── st7789 # ST7789驱动目录
│   │   ├── include # ST7789驱动头文件目录
│   │   ├── src # ST7789驱动源文件目录
│   │   └── ...... # 其他设备相关文件
│   └── ...... # 其他设备相关目录
│
├── template # 模板工程目录
│   ├── hello # hello工程目录
│   └── ...... # 其他模板工程目录
│
├── tools # 工具目录
│   ├── kconfig # 配置系统目录
│   ├── fixdep  # 配置系统工具
│   ├── scripts # 脚本目录
│   └── riscv # RISCCV工具链目录
│
└── ...... # 其他相关





