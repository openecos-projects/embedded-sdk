# am-kernels 上游记录

本目录是 SDK 内置的 ysyx am-kernels 工程模板集合。用户通过
`ecos init_project am-kernels/<程序路径>` 复制其中一个独立程序，不需要另外安装
或选择外部 am-kernels 路径。

- 上游路径：`/home/dallous/project/ysyx/am-kernels`
- 同步日期：2026-08-19
- 同步时源提交：`76c80f8b5b4fdeeabe1b7caa167953cd64c16545`
- 同步范围：benchmarks、kernels、tests、公共 README/LICENSE 和构建文件
- 排除项：`.git/`、各程序 `build/` 目录、`Makefile.html` 及其他生成物

同步时上游工作树包含 `kernels/bad-apple/Makefile` 和
`kernels/bad-apple/bad-apple.c` 的本地修改，副本按同步时状态保留。后续更新时
应先检查源工作树是否干净，再复制源码。包含标准 AM 应用入口的 Makefile 需要将
上游 `$(AM_HOME)/Makefile` include 替换为 ECOS 的 `application.mk`，以便实例化后
自动绑定 BSP 提供的 AM 环境；程序源码本身不做板卡适配。
