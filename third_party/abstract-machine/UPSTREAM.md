# AbstractMachine 上游记录

本目录是 ysyx AbstractMachine 的 SDK 迁入副本。

- 上游路径：`/home/dallous/project/ysyx/abstract-machine`
- 迁入日期：2026-08-19
- 迁入时源提交：`be82a3ffe9d836d71f8d7519285a53361431d339`
- 初始迁入范围：上游工作树中的公共头文件、AM 平台源码、Klib、构建脚本和工具
- 排除项：`.git/`、所有 `build/` 目录、`Makefile.html` 以及 ELF/BIN/HEX/TXT 生成物

SDK 当前只支持 StarrySkyL4 的 `riscv32e-ysyxsoc`，因此初始迁入后删除了不会进入
该构建路径的 LoongArch、MIPS、x86、native、NEMU、NPC、QEMU、Spike 和 rvmini
平台实现及脚本。NPC 目录中被 RV32E 复用的软算术源码保留为
`am/src/riscv/libgcc`；Klib 和 AM 公共头文件继续完整保留。原 ysyxsoc 的 CTE、旧
MMIO 设备实现和链接脚本也未保留，L4 使用 SDK HAL 与板级环境目录中的启动及
链接适配。

迁入时上游工作树并非干净状态。上游 `Makefile` 中的 `realpatffh` 拼写错误已在
SDK 适配阶段修复。同步时需要单独复核 SDK 的精简范围和本地适配差异。

后续同步时先将上游内容复制到临时目录，重新排除上述生成物和非 L4 架构，再单独
复核需要同步的公共文件。StarrySkyL4 的 HAL 适配放在
`board/StarrySkyL4/environments/abstract-machine`，平台构建和程序管控放在
`environments/abstract-machine`，不直接改写 SDK 内置 `am-kernels` 源码。
