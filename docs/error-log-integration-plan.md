# SDK 全局错误码与日志体系整合规划

## 1. 背景

当前 SDK 3.0 的 UART、GPIO、Timer 和 BSP 分别定义自己的错误码。虽然都遵守
`0` 表示成功、负数表示失败的基本约定，但相同数值在不同模块中可能表达不同语义。
例如 UART 的 `-3` 表示未初始化，GPIO 的 `-3` 表示 I/O 错误。调用方只拿到一个
`int` 时，无法对错误进行可靠的跨模块判断和统一输出。

现有 `TimmoLog` 也仍沿用独立模块和旧式输出链路：日志通过 `printf` 最终绑定到
`hal_sys_uart`，没有进入 SDK 3.0 的 BSP Console、公共 Driver 和 HAL 分层。

本规划将这两个问题一起解决：

- 建立 SDK 全局统一的错误码和错误语义 API。
- 将 TimmoLog 的日志能力迁入 SDK Core Runtime。
- 由 BSP Console 作为默认日志输出后端。
- 删除 TimmoLog 的独立模块身份和旧输出实现。

## 2. 目标与非目标

### 2.1 目标

- 所有新增 ECOS API 使用同一套 `ECOS_ERR_*` 错误码。
- 调用方可以统一判断成功、失败和具体错误语义。
- 错误值可以转换为稳定名称和适合终端显示的描述。
- 日志服务成为 SDK 默认能力，应用不需要显式选择 TimmoLog。
- 日志实现不直接依赖 UART、具体 Board 或旧 `printf` 输出路径。
- Console 输出保持适合人工阅读和自动化解析的稳定格式。
- 整套实现适用于无堆内存的裸机环境。

### 2.2 非目标

- 不修改 FatFs、SFUD 等第三方库自身的错误码定义。
- 第一阶段不实现错误原因链、堆栈回溯和运行时错误注册。
- 第一阶段不依赖系统时钟，因此日志默认不包含时间戳。
- 不在 HAL 和 Driver 内部自动打印所有错误，避免跨层重复日志。

## 3. 总体架构

```text
Application / BSP / Driver / HAL
                |
                | 返回统一 ecos_err_t
                v
       +-------------------+
       | SDK Core Runtime  |
       | ecos/error.h      |
       | ecos/log.h        |
       +-------------------+
                |
                | 可替换的 writer sink
                v
          BSP Console
                |
                v
          UART Driver -> UART HAL
```

Core Runtime 不依赖 BSP、Console 或 UART。BSP Console 初始化成功后，将自己的写函数
注册为默认日志 sink。这样 Driver/HAL 可以使用统一错误定义，但不会反向依赖 Console。

建议目录：

```text
components/core/
├── ecos-component.yml
├── include/ecos/error.h
├── include/ecos/log.h
├── src/error.c
└── src/log.c
```

建议构建身份：

```yaml
id: core-runtime
cmake_target: ecos::core::runtime
```

内部保留独立构建 target 是为了管理依赖和链接裁剪，但 Core Runtime 不作为应用需要
显式选择的可选模块。SDK 项目解析器应自动将它加入所有工程。

## 4. 全局错误码设计

### 4.1 基础类型和最小错误集

```c
typedef int32_t ecos_err_t;

enum {
    ECOS_OK                    = 0,
    ECOS_ERR_INVALID_ARGUMENT = -1,
    ECOS_ERR_UNSUPPORTED      = -2,
    ECOS_ERR_NOT_INITIALIZED  = -3,
    ECOS_ERR_IO               = -4,
    ECOS_ERR_TIMEOUT          = -5,
    ECOS_ERR_BUSY             = -6,
    ECOS_ERR_NO_MEMORY        = -7,
    ECOS_ERR_NOT_FOUND        = -8,
    ECOS_ERR_INVALID_STATE    = -9,
    ECOS_ERR_INTERNAL         = -10
};
```

错误码表达“发生了什么”，不编码 UART、GPIO、Timer 等来源。来源由日志 tag、操作名称
和调用上下文表达。这样所有模块都能直接进行相同的语义判断：

```c
if (result == ECOS_ERR_NOT_INITIALIZED) {
    /* 与错误来自 UART、Timer 还是 BSP 无关。 */
}
```

错误码数值属于公共 API，发布后不得在小版本中改变。只在出现真实需求时追加新错误码，
不预先建立大而空的错误表。

### 4.2 成功和失败判断

SDK 既有只返回状态的 API，也有返回字节数、实例数量或布尔状态的 API。因此：

- `ECOS_OK` 只适用于成功值明确为零的接口。
- 所有非负值均属于成功结果。
- 所有负值均属于错误结果。

公共辅助接口：

```c
bool ecos_result_succeeded(int result);
bool ecos_result_failed(int result);
bool ecos_err_is_known(int result);

const char *ecos_err_name(ecos_err_t error);
const char *ecos_err_description(ecos_err_t error);
```

预期行为：

| 输入 | `succeeded` | `failed` | 名称 |
| --- | --- | --- | --- |
| `0` | true | false | `ECOS_OK` |
| `12` | true | false | 不作为错误解析 |
| `ECOS_ERR_IO` | false | true | `ECOS_ERR_IO` |
| 未登记的负数 | false | true | `ECOS_ERR_UNKNOWN` |

辅助判断优先使用 `static inline` 函数，避免宏重复求值。名称和描述返回静态只读字符串，
不得分配内存。

### 4.3 各层使用规则

- HAL、Driver、Device Driver、BSP 的新接口统一返回 `ECOS_ERR_*`。
- 返回数据量或状态的函数继续使用 `int`，错误部分仍使用全局错误码。
- Driver 对已知 HAL 错误可以直接透传；无法对外表达的底层失败映射为
  `ECOS_ERR_IO` 或 `ECOS_ERR_INTERNAL`。
- 第三方错误只在 ECOS API 边界转换，第三方头文件和返回类型保持不变。
- 底层函数只返回错误，由最了解业务操作的上层决定是否记录日志。

## 5. SDK 日志服务设计

### 5.1 公共 API

```c
typedef enum {
    ECOS_LOG_DEBUG,
    ECOS_LOG_INFO,
    ECOS_LOG_WARN,
    ECOS_LOG_ERROR,
    ECOS_LOG_FATAL
} ecos_log_level_t;

typedef int (*ecos_log_writer_t)(
    void *context,
    const char *data,
    size_t size
);

ecos_err_t ecos_log_set_level(ecos_log_level_t level);
ecos_err_t ecos_log_set_writer(ecos_log_writer_t writer, void *context);

int ecos_log_write(ecos_log_level_t level,
                   const char *tag,
                   const char *file,
                   int line,
                   const char *format,
                   ...);

int ecos_log_error(const char *tag,
                   ecos_err_t error,
                   const char *operation,
                   const char *file,
                   int line);
```

应用主要使用宏封装：

```c
ECOS_LOGD("uart", "tx size=%u", size);
ECOS_LOGI("app", "startup complete");
ECOS_LOGW("timer", "counter is close to overflow");
ECOS_LOGE("storage", "mount failed");
ECOS_LOG_ERR("timer", result, "init timer 0");
```

`ECOS_LOG_ERR` 只接收已经求值的结果，宏不得重复执行表达式。首版不提供隐藏
`return`、`goto` 或死循环的检查宏；业务控制流保持显式。

### 5.2 错误输出格式

默认格式：

```text
[E][timer] init timer 0: ECOS_ERR_NOT_INITIALIZED (-3): resource not initialized
```

未知错误仍保留原始值：

```text
[E][storage] mount: ECOS_ERR_UNKNOWN (-37): unknown error
```

格式要求：

- 默认使用 ASCII 和 CRLF。
- 默认不带 ANSI 颜色，保证串口日志可以稳定解析。
- tag 和错误名称字段位置固定。
- 可通过配置附加源码文件和行号。
- 错误描述可以配置关闭，以节省固件只读数据空间。

### 5.3 运行时约束

- 使用固定大小栈缓冲区，不使用堆内存。
- sink 未注册时返回 `ECOS_ERR_NOT_INITIALIZED`，不得访问硬件。
- sink 返回短写时视为 `ECOS_ERR_IO`。
- 日志输出失败不得再次调用日志系统，避免递归。
- 第一阶段按裸机单线程模型实现；ISR 可用性必须明确标注，默认不承诺 ISR 安全。
- 编译期日志级别用于裁剪代码，运行时级别用于动态过滤。

`ECOS_LOG_FATAL` 只表达日志级别。停机操作由独立的 `ECOS_PANIC` 或平台 panic hook
负责，避免日志 API 隐式进入死循环。

## 6. Console 集成

BSP Console 保持板级资源职责。其初始化流程调整为：

1. 初始化绑定的 UART Driver。
2. 初始化 Console 自身换行状态。
3. 注册 BSP Console writer 为默认日志 sink。
4. 返回 `ECOS_OK`。

适配函数示意：

```c
static int console_log_writer(void *context,
                              const char *data,
                              size_t size)
{
    int result;

    (void)context;
    result = bsp_console_write(data, size);
    if (result < 0)
        return result;
    return (size_t)result == size ? result : ECOS_ERR_IO;
}
```

Console 不能使用日志系统报告自身底层写失败，否则会形成递归。Console 初始化失败时，
同一个 Console 也不具备输出该错误的物理条件；调用方仍可检查错误值，或提前注册调试器、
内存缓冲区等其他 sink。

## 7. TimmoLog 迁移

### 7.1 实现迁移

- 将日志级别、过滤和格式化能力迁入 `components/core`。
- 新实现只调用已注册 writer，不调用 `printf` 或 `hal_sys_uart`。
- 删除无实际作用的 filename 参数和 `log_close()`。
- 将无条件 ANSI 颜色改为配置项，默认关闭。
- 将 fatal 日志和永久循环拆分。

### 7.2 调用方迁移

将仓库内部调用统一替换：

| 旧接口 | 新接口 |
| --- | --- |
| `log_debug(...)` | `ECOS_LOGD(tag, ...)` |
| `log_info(...)` | `ECOS_LOGI(tag, ...)` |
| `log_warn(...)` | `ECOS_LOGW(tag, ...)` |
| `log_error(...)` | `ECOS_LOGE(tag, ...)` |
| `log_fatal(...)` | `ECOS_LOGF(tag, ...)` 或 `ECOS_PANIC(...)` |

迁移范围包括模板、FatFs/SFUD 的适配代码、Letter Shell port 和 libc assert。

迁移期间可以在 Core include 目录提供一个带 deprecated 标记的 `log.h` 兼容门面，
但不得保留第二套日志实现。仓库内部调用迁移完成后，在 SDK 3.0 正式发布前删除该门面。

### 7.3 清理旧模块

- 删除 `components/TimmoLog/include/log.h`。
- 删除 `components/TimmoLog/src/log.c`。
- 删除各 Board `build_conf.mk` 中对 `components/TimmoLog` 的搜索和 include 配置。
- 更新相关文档与示例中的 TimmoLog 名称。

## 8. 构建系统调整

当前项目解析器只把 Example 和 Board 声明的组件加入组件根集合。为了让 Core Runtime
真正成为 SDK 默认能力，建议扩展 SDK manifest：

```json
{
  "build": {
    "core_components": ["core-runtime"]
  }
}
```

项目解析顺序调整为：

```text
SDK core components
    + Example components
    + Board components
    -> 去重
    -> 解析完整依赖图
```

同时要求：

- Board 模式和 Target-only 模式都自动获得 Core Runtime。
- Driver/HAL 公共头包含 `ecos/error.h` 时，在 manifest 中声明正确依赖。
- 应用不需要在 `ecos-example.yml` 中显式添加 `core-runtime`。
- 链接器继续通过 section garbage collection 移除未使用的日志和错误描述代码。

## 9. 配置项

建议首版提供：

```text
CONFIG_ECOS_LOG_LEVEL
CONFIG_ECOS_LOG_BUFFER_SIZE
CONFIG_ECOS_LOG_COLOR
CONFIG_ECOS_LOG_SOURCE_LOCATION
CONFIG_ECOS_ERROR_DESCRIPTIONS
```

推荐默认值：

- 日志级别：`INFO`
- 缓冲区：根据实际格式化需求选取 128 或 256 字节
- ANSI 颜色：关闭
- 源码位置：Debug profile 开启，Release profile 关闭
- 错误描述：开启；对空间敏感的 profile 可关闭

## 10. 实施阶段

### 阶段一：Core Runtime 基础

- 创建 Core Runtime manifest、头文件和实现。
- 扩展 SDK manifest 与项目解析器，自动引入 Core Runtime。
- 完成错误判断、名称和描述 API。

### 阶段二：统一现有 3.0 错误码

- 迁移 UART HAL/Driver。
- 迁移 GPIO HAL/Driver。
- 迁移 Timer HAL/Driver。
- 迁移 Console、LED、Button BSP。
- 删除相互冲突的模块私有错误数值。

### 阶段三：日志和 Console

- 将 TimmoLog 实现迁入 Core Runtime。
- 完成 writer sink 和格式化输出。
- BSP Console 初始化成功后注册默认 sink。
- 更新 hello 示例展示错误判断和日志输出。

### 阶段四：存量迁移与清理

- 迁移所有 `log_*` 调用点。
- 处理 FatFs、SFUD、Letter Shell 和 assert 的兼容边界。
- 更新旧 Makefile 构建路径。
- 删除 `components/TimmoLog`。
- 在兼容窗口结束后删除旧 `log.h` 门面。

## 11. 测试计划

### 11.1 错误模型单元测试

- 零、正数、已知负数和未知负数的分类。
- 每个错误码名称和描述的映射。
- 错误码数值稳定性。
- 关闭描述配置后的构建行为。

### 11.2 日志单元测试

使用内存 writer 验证：

- 日志级别过滤。
- tag、错误名称、数值和源码位置格式。
- 未知错误输出。
- 缓冲区边界和截断。
- sink 未注册、短写和返回错误。
- 日志写失败不会递归。

### 11.3 构建与集成测试

- Target-only 工程自动包含 Core Runtime。
- Board 工程自动包含 Core Runtime 和 BSP Console。
- L4 hello/blink 工程继续生成 ELF、BIN 和反汇编。
- 链接结果包含所使用的错误/日志符号，未使用代码可被裁剪。
- 串口 monitor 能匹配稳定的错误日志文本。
- 旧 Makefile 工程在迁移窗口内仍能构建。

### 11.4 资源回归

记录迁移前后的：

- `.text` 大小。
- `.rodata` 大小。
- 最大栈缓冲区占用。
- 关闭详细错误描述和 Debug 日志后的裁剪效果。

## 12. 验收标准

- SDK 3.0 公共 API 不再各自声明冲突的 `*_ERROR_*` 数值。
- 所有 ECOS 层错误都能通过 `ecos_result_failed()` 判断。
- 已知错误能稳定转换为 `ECOS_ERR_*` 名称。
- 已初始化 Console 上能够输出结构化错误日志。
- Core Runtime 不直接依赖 UART、具体 Board 或旧 `hal_sys_uart`。
- 应用无需显式选择日志模块。
- 仓库内不再存在 `components/TimmoLog` 的独立实现和构建引用。
- 自动化测试覆盖错误语义、日志 sink 和 L4 完整构建链。
