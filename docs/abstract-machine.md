# AbstractMachine 工程使用说明

## 结构

AbstractMachine 是 SDK 的可选运行环境，不是程序集合。各部分职责如下：

- `third_party/abstract-machine`：AM 公共接口、Klib 和公共运行时源码。
- `environments/abstract-machine`：ECOS 的通用 AM 应用构建适配。
- `board/<board>/environments/abstract-machine`：BSP 的启动、链接、HAL 和架构实现。
- `templates/abstract-machine/common`：最小 AM 应用模板。
- `templates/am-kernels`：可复制的 am-kernels 应用、测试和 benchmark 模板。

am-kernels 程序只依赖 AM 接口，不包含板卡适配。任何在 `ecos-board.yml` 中登记了
`abstract_machine` 环境的 BSP 都可以实例化这些模板。BSP 是否完整实现某项 AM
设备接口，决定对应程序在硬件上的实际行为，不改变程序模板本身。

## 创建基础工程

```sh
eval "$(/path/to/embedded-sdk/bin/ecos env)"

ecos init_project abstract-machine \
    -name my_am_app \
    -target l4

cd my_am_app
make
```

生成的 `main.c` 是用户自己的 AM 程序。工程直接编译当前目录源码，不使用 `APP=`，
也不会从 SDK 暂存其他程序。最终产物为：

```text
build/retrosoc_fw.elf
build/retrosoc_fw.bin
build/retrosoc_fw.txt
build/retrosoc_fw.hex
```

## 从 am-kernels 创建工程

模板名直接对应 `templates/am-kernels` 下的程序路径：

```sh
ecos init_project am-kernels/kernels/hello \
    -name am_hello \
    -target l4

ecos init_project am-kernels/benchmarks/coremark \
    -name coremark_l4 \
    -target l4

ecos init_project am-kernels/benchmarks/microbench \
    -name microbench_l4 \
    -target l4
```

初始化器只复制选中的子工程。复制后的源码属于用户工程，可以直接修改，构建时不再
依赖 `templates/am-kernels`。运行 `ecos init_project list` 可以查看模板路径。

microbench 可以通过 AM 的 mainargs 选择规模：

```sh
make MAINARGS=test
```

## BSP 绑定

用户不选择 core 或架构。BSP 清单负责声明 AM 环境和默认 core：

```yaml
abstract_machine:
  path: environments/abstract-machine
  default_core: rv32e-base
```

创建工程时生成的 `configs/abstract-machine.mk` 记录 SDK、AM、BSP 环境路径和 BSP
默认 core。执行 `ecos set_board <board>` 可以改绑到另一个支持 AM 的 BSP，不会
替换用户的 Makefile 或源码。AM 工程当前不支持 `-isolated`。

## StarrySkyL4 当前实现

StarrySkyL4 默认使用 `rv32e-base`，编译参数为 `-march=rv32e -mabi=ilp32e`。
UART、Timer 和单核 MPE 已接入；GPU、Input、RTC、Audio、Disk、Net、VME 和 CTE
尚未形成完整硬件契约。使用这些接口的 am-kernels 模板仍可正常实例化，后续能力
补充应发生在 L4 BSP 的 AM 环境中，而不是修改 kernel 程序。

烧录入口和普通工程一致：

```sh
ecos flash
```
