# StarrySky_L3_1 内存使用模式配置指南

为了满足不同场景下的性能和成本需求，当前 StarrySky_L3_1 工程配置了两种不同的内存使用模式：**XIP模式** 和 **MEM模式**。你可以通过 `make menuconfig` 中的 `Memory Region Selection` 菜单在这两种模式之间进行切换。

---

## 1. 模式介绍与区别

### 1.1 XIP 模式 (Execute In Place)
- **概念**：片内执行模式。代码和只读数据直接存储在非易失性存储器 ROM (Flash) 中，并在 ROM 中直接执行。可变数据（如 `.data` 和 `.bss` 段）以及系统栈 (Stack) 则被分配在高速 RAM（即 MEM）中。
- **优点**：最大限度地节省了 RAM (MEM) 的占用；启动速度快（无需等待 Bootloader 将大量代码搬运到 RAM）。
- **缺点**：执行速度受限于 ROM (Flash) 的读取带宽和延迟，性能略低。
- **配置选项**：`CONFIG_LINK_TARGET_XIP`

### 1.2 MEM 模式 (RAM 运行模式)
- **概念**：代码和数据在运行前全部被搬运到 RAM 中。程序烧录时仍然保存在 ROM (Flash) 中，但系统上电后，内置的 Bootloader 会先将应用程序的全部内容拷贝到高速 RAM (MEM) 中，然后跳转到 RAM 中以全速执行。
- **优点**：执行速度极快，没有 Flash 读取的等待周期（Zero Wait States），适合对实时性要求极高的核心算法或中断响应。
- **缺点**：非常消耗 RAM 容量；系统启动时会因为增加了一段搬运过程而略微变慢。
- **配置选项**：`CONFIG_LINK_TARGET_MEM`

---

## 2. 配置文件变更说明

为支持这两种模式的无缝切换，工程中的以下配置文件已被重构和优化：

1. **`board.kconfig` (Kconfig 菜单配置)**
   - 添加了 `LINK_TARGET_XIP` 和 `LINK_TARGET_MEM` 的选择菜单。
   - 将默认的 RAM 名称标识统一修改为 `MEM`。
   - 明确了默认的 ROM (Flash) 容量为 16MB，MEM (RAM) 容量为 128MB。

2. **`build_conf.mk` & `Makefile` (编译与构建)**
   - 根据 Kconfig 传入的配置，动态向 C 编译器和链接器传入 `-DCONFIG_LINK_TARGET_XIP` 或 `-DCONFIG_LINK_TARGET_MEM` 宏。
   - 在 XIP 模式下，直接生成最终的镜像并跳过 Bootloader 的编译。
   - 在 MEM 模式下，将应用镜像 `app.bin` 注入到 Bootloader (`loader`) 中生成整合镜像。

3. **`sections.lds` (链接脚本)**
   - 重构了链接脚本，显式定义了 `MEMORY { ROM ... MEM ... }` 的内存块布局。
   - 利用 C 预处理器宏动态映射各个段。
   - **XIP 模式时**：`.text` 和 `.rodata` 映射到 `ROM`，`.data` 加载地址在 `ROM` 但虚拟运行地址在 `MEM`，`.bss` 和堆栈分配在 `MEM`。
   - **MEM 模式时**：所有段（代码、数据、堆栈）均映射到 `MEM`。

4. **`start.S` (启动汇编)**
   - 增加了从 ROM 拷贝 `.data` 段到 MEM 的搬运循环（仅在 XIP 模式下生效）。
   - 增加了对 `.bss` 段清零的初始化逻辑，确保未初始化的全局变量为零。

5. **`loader.c` (Bootloader)**
   - 修复了加载目标地址，确保在 MEM 模式下 Bootloader 准确地将应用解包至 `0x80000000` (MEM 基址) 并跳转。

---

## 3. 使用方法

1. 进入项目根目录。
2. 运行 `make menuconfig` 打开配置菜单。
3. 导航到 `Target Hardware Configuration` -> `Memory Region Selection`。
4. 按 `Y` 或 `Space` 键选择你需要的模式：
   - `[ ] XIP Mode (ROM for Code, MEM for Data)`
   - `[ ] MEM Mode (All in MEM)`
5. 保存配置并退出。
6. 运行 `make clean`（或 `make clean_all`）清除旧的构建产物。
7. 运行 `make` 开始编译。
8. 编译完成后，会自动打印最终产物的内存占用报告，你可以通过报告验证代码是链接到了 `FLASH` 还是 `MEM` 中。