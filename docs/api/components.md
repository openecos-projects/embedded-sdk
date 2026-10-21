# 应用向组件

本文记录 `components/` 下可供应用使用的组件：libc 子集、heap、
LightCoroutine 协程、sfud 串口 Flash 驱动、FatFs 文件系统、Letter-Shell 命令行、
文件系统命令、GPIO bit-bang SPI、终端文本编辑器和小型 C 编译器。
`core-runtime` 与 `libc` 子集是 SDK 默认构建单元；LightCoroutine、sfud、FatFs、
Letter-Shell、letter-shell-fs、soft-spi、text-editor、fcc 需要由 Example 声明组件 ID。
上述组件均随 SDK 3.0 release 包发布；真实 Flash 挂载、文件读写、终端交互和掉电恢复
仍需按板卡逐项完成实板验收，结果记录在各示例 README 和发布验收清单中。

不记录：`soc`（SDK 内部 SoC 层）和 `libgcc`（编译器内部例程）。旧版固定引脚
`spi_software` 已移除；3.0 应使用下述 `soft-spi`。

## libc 子集

头文件：`components/libc/include/`（`stdio.h`、`string.h`、`assert.h`）。
输出走 sys UART（控制台串口），不经文件系统。

stdio.h：

```c
int printf(const char *fmt, ...);
int puts(const char *str);
int vprintf(const char *fmt, va_list args);
int vsnprintf(char *str, size_t size, const char *fmt, va_list args);
int snprintf(char *buffer, size_t size, const char *format, ...);
```

string.h：

```c
void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* s, int c, size_t n);
size_t strlen(const char* s);
size_t memcmp(const void* ptr1, const void* ptr2, size_t num);  /* 注意：返回 size_t，非标准 */
char* strchr(const char* str, int c);
int strcmp(const char* src, const char* dst);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, size_t len);
int strncmp(const char* s1, const char* s2, size_t n);
void* memmove(void* dest, const void* src, size_t num);
```

assert.h：

```c
assert(expr, fmt, ...);
```

带格式化消息的断言：**仅在 `CONFIG_BUILD_DEBUG` 下生效**（失败时经
`ECOS_PANIC` 打印并停机），发布构建中展开为 `((void)0)`。与标准 C 的
`assert` 不同，必须提供 `fmt` 参数。

注意：这是 libc **子集**，没有文件 IO、浮点格式化等；`memcmp` 的
返回类型是 `size_t`（标准 C 为 `int`），比较结果请只判断是否为 0。
堆分配由独立的 heap 组件提供（见下节）。

## heap（动态内存分配）

头文件：`components/heap/include/ecos/heap.h`。

双区堆模型，对齐商用芯片的内部/外部 RAM 分工：

- `ECOS_HEAP_ZONE_INTERNAL`：片上 SRAM 池，延迟低、启动即可用，
  `ecos_heap_malloc()` 的默认去向；
- `ECOS_HEAP_ZONE_EXTERNAL`：外部大容量内存池（如 SDRAM），用于显式申请
  的大缓冲（帧缓冲、算法工作集）。

```c
typedef enum {
    ECOS_HEAP_ZONE_INTERNAL = 0,
    ECOS_HEAP_ZONE_EXTERNAL
} ecos_heap_zone_t;

ecos_err_t ecos_heap_add_pool(uintptr_t base, size_t size,
                              ecos_heap_zone_t zone);
void *ecos_heap_malloc(size_t size);
void *ecos_heap_alloc(ecos_heap_zone_t zone, size_t size);
void *ecos_heap_calloc(ecos_heap_zone_t zone, size_t count, size_t size);
void *ecos_heap_realloc(void *ptr, size_t size);
void ecos_heap_free(void *ptr);
size_t ecos_heap_free_bytes(ecos_heap_zone_t zone);
```

- 内存池由 Target 启动代码在 `main` 之前注册；未注册时分配返回 `NULL`。
- 分配器为 K&R 风格首次适配空闲链表，4 字节对齐，`free` 合并相邻空闲块。
- 单线程裸机模型：接口无锁，不要在中断与主流程中并发调用。
- `ecos_heap_free_bytes()` 返回该 zone 的空闲总量（含碎片，单次最大分配
  可能更小）。
- 目前只有 StarrySky T1-Pico 在启动时注册池：INTERNAL = DSRAM 堆区，
  EXTERNAL = 32 MiB SDRAM（启动代码同时完成 SDRAM 控制器初始化）。
  其他 Target 上分配返回 `NULL`。

## LightCoroutine（轻量协程）

3.0 组件 ID 为 `light-coroutine`，示例
[coroutine-smoke](../../examples/concurrency/coroutine-smoke) 覆盖任务创建、
上下文保存、让出和有限轮次调度。组件不依赖具体外设；需要调整任务数量或
上下文大小时，在 `ecos menuconfig` 中启用对应的 `COMPONENT_COROUTINE_*`
配置。

头文件：`components/LightCoroutine/include/coroutine.h`、`yield.h`，
配置见 `conf.h`。组件源码使用 GCC 的 computed-goto 扩展，SDK 3.0 的
RISC-V GCC 工具链已覆盖该语法。
示例：`examples/concurrency/coroutine-smoke`。

基于 `goto *label`（GCC 计算 goto）的栈less协作式协程，适合在裸机单线程上
组织多个逻辑任务。

### 任务 API

```c
typedef struct task_t {
    int id;
    void* resume_point;
    void (*func)(struct task_t*);
    void *args;
} task_t;

typedef void (*func_t)(task_t*);

task_t create_task(int id, func_t func, void* args);
void delete_task(task_t* task_handler);
void suspend_task(task_t* task_handler);
void resume_task(task_t* task_handler);
void task_scheduler(int round);   /* round = 调度轮数；-1 永久运行 */
```

### 编程模型宏（yield.h）

```c
TASK_BEGIN(t)               /* 任务函数开头，恢复断点 */
TASK_YIELD(t)               /* 主动让出 */
TASK_WAIT(t, cond)          /* 条件不满足则挂起等待 */
TASK_END(t)                 /* 任务结束并注销 */
TASK_YIELD_DETAILED(t, post)                  /* 带善后语句的让出 */
TASK_WAIT_DETAILED(t, pre, cond, post_false, post_true)  /* 带前置/后置语句的等待 */
LOCAL(...)                  /* 声明跨切出保持的局部变量（需 USE_CTX） */
```

任务函数必须按 `TASK_BEGIN` … `TASK_END` 包裹；`TASK_YIELD`/`TASK_WAIT` 之后的
普通局部变量在切出后丢失，需要保持的变量用 `LOCAL(...)` 声明（经
`local.<field>` 访问）。

### 配置（conf.h）

| 宏 | 默认 | 说明 |
| --- | --- | --- |
| `MAX_TASK_NUM` | 10 | 最大任务数 |
| `USE_CTX` | 定义 | 启用上下文保存（`LOCAL` 生效） |
| `TASK_CTX_SIZE` | 64 | 每任务上下文大小（uint32_t 个数） |
| `USE_IDLE_TASK` | 定义 | 启用空闲任务 |

menuconfig 对应项：`CONFIG_COMPONENT_COROUTINE*`。

### 骨架

```c
#include "coroutine.h"

static task_t handler;

static void blink_task(task_t *t)
{
    LOCAL();
    TASK_BEGIN(t);
    for (;;) {
        /* …工作… */
        TASK_YIELD(t);
    }
    TASK_END(t);
}

int main(void)
{
    handler = create_task(0, blink_task, NULL);
    task_scheduler(-1);   /* 永久调度，不返回 */
    return 0;
}
```

需要更完整的协作式内核骨架时，可参考示例
[mini-os](../../examples/os/mini-os)：它在 LightCoroutine 之上实现静态任务表、
round-robin 调度、1 ms 轮询 tick、sleep/wake、挂起/恢复，并把 Letter-Shell
作为普通任务接入，提供 `ps`、`uptime`、`spawn`、`kill` 等命令。

## sfud（串口 Flash 通用驱动）

3.0 组件 ID 为 `sfud`，需要 Board 提供 `qspi` 和 `console` 能力。示例
[flash-sfud](../../examples/storage/flash-sfud) 覆盖初始化与设备识别；实板
擦写、校验和掉电恢复仍按人工验收清单执行。

头文件：`components/sfud/include/sfud.h`，配置见 `sfud_cfg.h`。

标准 SFUD 库（Armink）：自动识别 SPI NOR Flash 并提供统一读写接口。

```c
sfud_err sfud_init(void);
sfud_err sfud_device_init(sfud_flash *flash);
sfud_flash *sfud_get_device(size_t index);
size_t sfud_get_device_num(void);
sfud_flash *sfud_get_device_table(void);

sfud_err sfud_read(const sfud_flash *flash, uint32_t addr, size_t size, uint8_t *data);
sfud_err sfud_erase(const sfud_flash *flash, uint32_t addr, size_t size);
sfud_err sfud_write(const sfud_flash *flash, uint32_t addr, size_t size,
                    const uint8_t *data);
sfud_err sfud_erase_write(const sfud_flash *flash, uint32_t addr, size_t size,
                          const uint8_t *data);
sfud_err sfud_chip_erase(const sfud_flash *flash);
sfud_err sfud_read_status(const sfud_flash *flash, uint8_t *status);
sfud_err sfud_write_status(const sfud_flash *flash, bool is_volatile, uint8_t status);
```

- 错误类型 `sfud_err`：`SFUD_SUCCESS = 0`、`SFUD_ERR_NOT_FOUND`、
  `SFUD_ERR_WRITE`、`SFUD_ERR_READ`、`SFUD_ERR_TIMEOUT`、
  `SFUD_ERR_ADDR_OUT_OF_BOUND` 等。
- 设备对象类型 `sfud_flash`；`sfud_init()` 探测并初始化设备表中的所有 Flash。
- SDK 配置（`sfud_cfg.h`）：启用 `SFUD_USING_SFDP`；`SFUD_FLASH_DEVICE_TABLE`
  预置一块 W25Q64CV @ SPI0。

## fatfs（FatFs 文件系统）

3.0 组件 ID 为 `fatfs`，需要 Board 提供 `qspi` 和 `console` 能力；示例
[flash-fatfs](../../examples/storage/flash-fatfs) 只做非破坏性挂载探测。
头文件：`components/fatfs/include/ff.h`（ChaN FatFs R0.16），SDK 胶水：`ffinit.h`。

标准 FatFs API（节选）：

```c
FRESULT f_open(FIL* fp, const TCHAR* path, BYTE mode);
FRESULT f_close(FIL* fp);
FRESULT f_read(FIL* fp, void* buff, UINT btr, UINT* br);
FRESULT f_write(FIL* fp, const void* buff, UINT btw, UINT* bw);
FRESULT f_lseek(FIL* fp, FSIZE_t ofs);
FRESULT f_sync(FIL* fp);
FRESULT f_opendir(DIR* dp, const TCHAR* path);
FRESULT f_readdir(DIR* dp, FILINFO* fno);
FRESULT f_mkdir(const TCHAR* path);
FRESULT f_unlink(const TCHAR* path);
FRESULT f_rename(const TCHAR* path_old, const TCHAR* path_new);
FRESULT f_stat(const TCHAR* path, FILINFO* fno);
FRESULT f_mount(FATFS* fs, const TCHAR* path, BYTE opt);
FRESULT f_mkfs(const TCHAR* path, const MKFS_PARM* opt, void* work, UINT len);
FRESULT f_getfree(const TCHAR* path, DWORD* nclst, FATFS** fatfs);
```

核心类型：`FATFS`（卷对象）、`FIL`（文件对象）、`DIR`（目录对象）、
`FILINFO`（文件信息）、`FRESULT`（结果码）。完整语义见 FatFs 官方文档。

SDK 胶水（`ffinit.h`）：

```c
bool load_filesystem(void);   /* 只挂载已有卷，不改变 Flash 内容 */
FRESULT format_filesystem(void); /* 显式格式化并挂载，会擦除卷 */
```

应用通常只需 `load_filesystem()`，成功后直接用 `f_open` 等标准 API。找不到文件系统时
不会自动格式化；需要初始化空 Flash 时，由应用显式调用 `format_filesystem()`，并先确认
卷内数据已备份。
注意 FatFs 的 `FIL` 等对象较大，栈上分配时注意栈余量。

## letter-shell（命令行 Shell）

3.0 组件 ID 为 `letter-shell`，需要 Board 提供 `console` 能力；示例
[letter-shell-smoke](../../examples/concurrency/letter-shell-smoke) 覆盖命令表初始化、
公共控制台收发回调和 `help` 命令解析。组件随 3.0 release 提供；串口交互、
长时间输入、退格/历史/方向键和边界输入仍需按板卡完成实板验收。

头文件：`components/letter-shell/include/shell.h`，
一键启动：`shell_init.h`，移植钩子：`shell_port.h`。
`shell_fs.h` 是 Letter-Shell 内部文件系统伴侣接口；应用层文件系统命令见下节。

### 核心 API（shell.h）

```c
void shellInit(Shell *shell, char *buffer, unsigned short size);
void shellTask(void *param);
void shellHandler(Shell *shell, char data);
void shellPrint(Shell *shell, const char *fmt, ...);
int shellRun(Shell *shell, const char *cmd);
```

- `Shell` 对象先清零，再设置 `read`/`write`，最后调用 `shellInit()`；
  有文件系统路径时，可在初始化前用 `ecos_shell_fs_init()` 设置路径。
- `shellInit()` 绑定收发缓冲；`shellHandler()` 逐字节喂入；
  `shellTask()` 为轮询任务体；`shellPrint()` 向终端格式化输出；
  `shellRun()` 以字符串执行一条命令。
- 命令导出宏 `SHELL_EXPORT_CMD`（及 `_SIGN`/`_AGENCY` 变体）：把函数注册为
  shell 命令。**当前 `shell_cfg.h` 未启用 CMD_EXPORT**
  （`SHELL_USING_CMD_EXPORT = 0`），导出宏展开为空，可用命令集合是编译
  内置的。
- 默认用户 `StarrySky`，密码为空。

### 一键启动（shell_init.h）

```c
void load_shell();
```

初始化 shell 对象和缓冲并进入主循环，不返回。需要文件系统命令的应用应先挂载
FatFs 卷并初始化 `letter-shell-fs`，按 `text-editor` 示例注册自己的命令表。

### 移植钩子（shell_port.h）

```c
short shellRead(char* str, unsigned short len);
short shellWrite(char* str, unsigned short len);
```

shell 的字节收发钩子，SDK 已对接系统串口。

## letter-shell-fs（文件系统命令）

3.0 组件 ID 为 `letter-shell-fs`，依赖 `letter-shell` 和 `fatfs`。头文件：
`components/letter-shell-fs/include/ecos/shell/fs.h`。示例
[text-editor](../../examples/storage/text-editor) 在挂载已有卷后注册 `cd`、`ls`、
`touch` 和 `edit` 命令。

```c
char path[ECOS_SHELL_FS_PATH_CAPACITY];
ecos_shell_fs_init(&shell, path, sizeof(path)); /* 在 shellInit 前调用 */
```

`cd <directory>` 更新 FatFs 当前目录及 Shell 提示符，`ls [directory]` 逐项列出文件，
`touch <filename>` 创建不存在的文件，已有文件内容不会被截断。命令入口分别为
`ecos_shell_fs_cd()`、`ecos_shell_fs_ls()`、`ecos_shell_fs_touch()`；应用通过自己的
Letter-Shell 命令表注册它们。该组件目前只用于单一活动 Shell 和单一 FatFs 卷，
仍需实板验收。

`ShellFs` / `shellFsInit` 由 Letter-Shell 内部实现使用，应用应通过
`letter-shell-fs` 暴露的 `ecos_shell_fs_*` 接口访问文件系统命令。

## soft-spi（GPIO bit-bang SPI）

3.0 组件 ID 为 `soft-spi`，依赖 `driver-gpio`。头文件：
`components/soft_spi/include/ecos/soft_spi.h`。它替代旧的固定引脚实现。Board 通过 `soft-spi-bus` 资源声明四个
引脚和 mode，配置阶段生成 `ecos/board_resources.h` 宏，应用不能把板卡引脚写进组件。

```c
#include "ecos/board_resources.h"
#include "ecos/soft_spi.h"

ecos_soft_spi_config_t config = {
    .chip_select = ECOS_BOARD_SOFT_SPI_CHIP_SELECT,
    .clock = ECOS_BOARD_SOFT_SPI_CLOCK,
    .mosi = ECOS_BOARD_SOFT_SPI_MOSI,
    .miso = ECOS_BOARD_SOFT_SPI_MISO,
    .mode = (ecos_soft_spi_mode_t)ECOS_BOARD_SOFT_SPI_MODE,
};
ecos_soft_spi_t bus;
uint8_t received;

ecos_soft_spi_init(&bus, &config);
ecos_soft_spi_start(&bus);
ecos_soft_spi_transfer_byte(&bus, 0x9fu, &received);
ecos_soft_spi_stop(&bus);
```

支持 SPI mode 0 到 mode 3、MSB-first 单字节收发和可选每半周期回调延时；
事务必须显式 `start`/`stop`，调用方负责选择与硬件匹配的 GPIO 资源。宿主测试覆盖
四种 mode、引脚冲突和状态机；具体板卡时钟频率、外设电气连接和长时间稳定性仍需实板验收。

## text-editor（终端文本编辑器）

3.0 组件 ID 为 `text-editor`，依赖 `fatfs` 和 `letter-shell`，要求 Target 声明
`ram-64k` 能力。编辑器状态约占 42 KiB 静态 RAM；当前示例
[text-editor](../../examples/storage/text-editor) 支持 C1、C2、L3.1 和 L4 C1/C2/C3。
T1-Pico 片内 RAM 不足，L3 缺少示例所需的 QSPI 资源。

入口 `int editCmd(int argc, char *argv[]);` 接受 `edit <filename>`；示例挂载已有
FatFs 卷，并在自己的 Letter-Shell 命令表注册 `edit` 和文件系统命令。编辑器使用 BSP console
处理输入和 ANSI 屏幕输出，不直接访问 UART 寄存器；默认终端尺寸为 80 列、24 行。
支持方向键、回车、退格、Tab、`Ctrl+W` 保存和 `Esc` 退出；有未保存内容时需再次
按 `Esc` 才会丢弃。单文件最多 24 KiB、8192 行，超限时拒绝加载或插入。
示例不会自动格式化空 Flash；格式化前须先备份卷内数据。

组件和示例随 3.0 release 提供；保存、重启后读取、终端兼容性及掉电行为需要实板验收。

## fcc（小型 C 编译器）

3.0 组件 ID 为 `fcc`，依赖 `fatfs`、`letter-shell`，要求 Target 声明
`rv32im-exec-sram`；当前只有 C2 支持。[fcc 示例](../../examples/storage/fcc) 挂载已有
FatFs 卷，在 Shell 中注册 `fcc <source.c>` 和 `fcc-run`。前者把源码编译到链接脚本
预留的 32 KiB 片内 SRAM，后者执行最近一次成功编译的无参 `main`。
源码上限 32 KiB，全局变量区 1 KiB；编译失败会禁止执行上一次生成的代码。
它只实现整数、函数、条件、循环和少量运算符，不是完整 C 编译器。

两个 C2 启动 profile 已完成交叉构建和地址检查；实板运行、栈余量与文件系统读写
仍需按 C2 板卡和人工验收清单确认。组件和示例随 3.0 release 提供。
