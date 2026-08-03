# 分离式工程

## 功能
生成分离式工程（Isolated Project）时，会按照板卡类型复制 BSP、HAL、组件源码、构建脚本、Kconfig 和 fixdep。生成后不再读取 SDK 原目录，但 RISC-V 交叉编译工具链仍然是不可缺少的组件。

## 适配板卡类型
目前适配C2、L3.1板卡。

## 项目结构
ECOS_Isolated_Project/
├── README.md          # 项目说明文档
├── Makefile           # 构建配置
├── scripts/           # 本地构建脚本
├── tools/             # 本地 Kconfig 和 fixdep 工具
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
# 1. 创建项目
ecos init_project <项目名称> -isolated -name <项目新名称> -target <板卡类型>
# 2. 根据项目需求，将项目中不需要的组件裁剪掉
# 3. 根据项目需求，将项目中需要的宏开关打开
make menuconfig 
# 4. 编译（可以脱离 SDK 原目录）
make
```

1. `make defconfig`、`make menuconfig` 和 `make` 都使用工程内的 `tools/` 与 `scripts/`，不需要设置 `ECOS_SDK_HOME`。
2. 工程不会复制 SDK 中已有的 Kconfig/fixdep 构建产物。首次配置需要主机 `gcc`、`g++`、`make`、`flex` 和 `bison`，menuconfig 还需要 ncurses 开发库。
3. 工程不会复制交叉编译工具链，用户仍需将工具链加入 `PATH`，或按 Makefile 要求设置工具链路径。
4. 调整menuconfig中的Peripheral Drivers与External Devices只会影响对应宏的生成
  - 在分离式项目中，调整它只会影响到对应宏的生成，不会影响到头文件包含
  - 在分离式项目中，它们均默认加入到CFLAGS的头文件搜索路径中
  - 对于不想使用的组件，您可以直接裁剪

## 新版卡的适配方式
注意，板卡资源仅包含板卡相关的文件、文件夹（SDK/board下的资源）

1. 在ecos-set_board_isolated.sh的set_board函数中，编写“移动新板卡的相关文件”的逻辑
2. 执行./install.sh更新SDK

## 新SDK资源的适配方式
注意，SDK资源仅包含板卡不相关的文件、文件夹（SDK/board以外的资源）

1. 在ecos-init_project.sh的init_project_isolated函数中，编写“移动SDK新资源”的逻辑
2. 执行./install.sh更新SDK
