# 应用向组件

本文记录 `components/` 下可供应用直接使用的组件：libc 子集、LightCoroutine
协程、sfud 串口 Flash 驱动、fatfs 文件系统、letter-shell 命令行，以及以 shell
命令形式存在的内置应用 fcc / text_editor。

不记录：`soc`（SDK 内部 SoC 层）、`libgcc`（编译器内部例程）、`spi_software`
（2.x 遗留，未移植到 3.x 契约）。

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

注意：这是 libc **子集**，没有堆分配、文件 IO、浮点格式化等；`memcmp` 的
返回类型是 `size_t`（标准 C 为 `int`），比较结果请只判断是否为 0。

## LightCoroutine（轻量协程）

头文件：`components/LightCoroutine/include/coroutine.h`、`yield.h`，
配置见 `conf.h`。
示例：`templates/coroutine_test`。

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

## sfud（串口 Flash 通用驱动）

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

头文件：`components/fatfs/include/ff.h`（ChaN FatFs R0.16），
SDK 胶水：`ffinit.h`。需要启用 `CONFIG_COMPONENT_FLASH_FS`。

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
bool load_filesystem();   /* 经 sfud 挂载板载 Flash 上的文件系统 */
```

应用通常只需 `load_filesystem()`，成功后直接用 `f_open` 等标准 API。
注意 FatFs 的 `FIL` 等对象较大，栈上分配时注意栈余量。

## letter-shell（命令行 Shell）

头文件：`components/letter-shell/include/shell.h`，
一键启动：`shell_init.h`，移植钩子：`shell_port.h`，
文件系统伴侣：`shell_fs.h`。
示例：`templates/shell_test`。

### 核心 API（shell.h）

```c
void shellInit(Shell *shell, char *buffer, unsigned short size);
void shellTask(void *param);
void shellHandler(Shell *shell, char data);
void shellPrint(Shell *shell, const char *fmt, ...);
int shellRun(Shell *shell, const char *cmd);
```

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

完成 shell 对象、缓冲、（可选）文件系统伴侣的全部初始化并进入 shell 主
循环，不返回。

### 移植钩子（shell_port.h）

```c
short shellRead(char* str, unsigned short len);
short shellWrite(char* str, unsigned short len);
```

shell 的字节收发钩子，SDK 已对接系统串口。

### 文件系统命令（shell_fs.h，需 CONFIG_COMPONENT_FLASH_FS）

```c
void shellFsInit(ShellFs *shellFs, char *pathBuffer, size_t pathLen);
void shellCD(char *dir);
void shellLS(void);
void shellTOUCH(char* filename);
```

向 shell 注册 `cd`/`ls`/`touch` 等文件命令；`load_shell()` 在启用
FLASH_FS 时会自动挂上。

## 内置应用（shell 命令形式）

fcc 与 text_editor 不是库，而是以 shell 命令形式存在的内置应用：

- `components/fcc` — 玩具 C 编译器，入口 `int fccCmd(int argc, char **argv);`；
- `components/text_editor` — 终端文本编辑器（ANSI 界面，FatFs 存取），入口
  `int editCmd(int argc, char **argv);`。

进入 shell 后直接以命令名调用（如 `edit <文件名>`），应用代码一般不直接
链接这两个入口。
