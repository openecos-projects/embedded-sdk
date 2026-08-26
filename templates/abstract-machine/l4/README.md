# StarrySkyL4 AbstractMachine 工程

运行 `make` 查看当前核心能力和程序清单，使用
`make APP=kernels/hello` 构建最小程序。microbench 首次验证推荐使用
`make APP=benchmarks/microbench MAINARGS=test`；coremark 和 dhrystone 也可从
`make list` 所示清单直接构建。所有暂存源码、对象和镜像均写入本工程的
`build/`，不会修改 SDK 内置的 am-kernels 或 SDK 其他源码。

完整说明见 SDK 的 `docs/abstract-machine.md`。
