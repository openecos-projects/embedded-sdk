# Embedded SDK HAL 层 V2.0 设计理念与 API 弃用规范

## 1. 背景与初衷
随着项目的演进，为了提升 SDK 的可移植性、可扩展性和可维护性，我们需要对现有的硬件抽象层（HAL）进行重构。
业界优秀的框架（如 ESP-IDF 的 LL -> HAL -> Driver 三层架构）虽然功能强大，但对于目前人手不足、工程不够庞大复杂的团队来说，完全照搬会导致“过度设计”，增加维护成本和学习曲线。

因此，V2.0 版本的 HAL 层设计采取**“演进式架构（Evolutionary Architecture）”**策略：在保留核心接口契约（API Signature）对标业界标准的前提下，尽可能简化内部实现，以便未来平滑过渡到完整的架构体系中。

---

## 2. V2.0 HAL 层设计理念 (Minimal & Scalable)

V2.0 版本的核心原则是**“保留未来扩展的接口，砍去当前不需要的复杂度”**。

### 2.1 保留的核心要素（为了未来不改接口）
* **保留“端口/实例（Port/Instance）”概念**：
  所有外设操作 API 第一个参数必须是外设实例 ID（如 `hal_uart_port_t`），不要在内部将操作绑定在特定硬件实例上。这保证了未来扩展多路实例时应用层代码无需改动。
* **保留“配置结构体（Config Struct）”**：
  初始化参数统一通过结构体传入。若硬件新增功能（如流控），只需在结构体增加字段，保证了 `init` 函数接口的前向兼容。
* **保留“统一的错误码”**：
  API 的返回值统一为 `int`，约定 `0` 为成功，`< 0` 为错误码。废弃无返回值的 `void` 类型接口。

### 2.2 砍掉的非必要特性（为了降低开发维护成本）
* **砍掉严格的 LL/HAL/Driver 物理分层**：
  目前只需保留一层。在 `hal_xxx.c` 的实现中直接操作底层寄存器或原厂驱动，不强制要求分离出独立的 LL（底层寄存器）文件。
* **砍掉复杂的异步、事件回调、DMA 抽象**：
  如果当前业务无需使用复杂的 RTOS 调度，直接提供“阻塞式（轮询/超时）”读写即可，未来可再补充带回调的非阻塞接口。
* **砍掉过度细化的文件拆分**：
  相关类型、寄存器、结构体等定义，可以直接整合在一个 `hal_xxx.h` 文件中，避免跨文件查阅带来的理解成本。

### 2.3 设计示例参考 (以 UART 为例)
```c
#ifndef __HAL_SYS_UART_H__
#define __HAL_SYS_UART_H__

#include <stdint.h>
#include <stddef.h>

// 1. 类型定义
typedef enum {
    HAL_UART_PORT_0 = 0,
    HAL_UART_PORT_1,
    HAL_UART_PORT_MAX
} hal_uart_port_t;

typedef struct {
    uint32_t baud_rate;
    // V2.0如果仅需 8N1，可以暂不开放以下参数，保持极简
    // uint8_t data_bits; 
    // uint8_t stop_bits;
} hal_uart_config_t;

// 2. 核心 API
int hal_uart_init(hal_uart_port_t port, const hal_uart_config_t *config);
int hal_uart_deinit(hal_uart_port_t port);

// 返回实际发送/接收的字节数，或 <0 的错误码
int hal_uart_write(hal_uart_port_t port, const uint8_t *data, size_t len);
int hal_uart_read(hal_uart_port_t port, uint8_t *data, size_t max_len, uint32_t timeout_ms);

#endif // __HAL_SYS_UART_H__
```

---

## 3. API 弃用 (Deprecation) 标准流程

为了在引入 V2.0 架构时不破坏现有业务代码，所有旧 API 必须严格按照以下四个阶段进行弃用过渡，保证项目的稳定性。

### 阶段一：设计并引入新 API (New API Introduction)
* **动作**：编写并提交新的 `hal_xxx` 接口定义与底层实现。
* **老接口处理**：**保留老接口的声明不删除**，将其内部实现修改为调用新 API 的包装器（Wrapper）。
* **目的**：保证所有依赖老接口的代码无需修改即可运行，但底层逻辑已统一切换至新架构。

### 阶段二：软弃用与编译器警告 (Soft Deprecation & Warnings) - **当前 V2.0 重点执行阶段**
* **动作**：利用编译器特性（如 GCC/Clang 的 `__attribute__((deprecated))`）将老接口标记为弃用状态。
* **目的**：当开发者在编写新代码时不小心调用了老接口，编译器会发出 Warning 并提示应该使用的替代接口，但**不阻断编译**。

**代码示例**：
```c
#ifndef __DEPRECATED
  #if defined(__GNUC__) || defined(__clang__)
    #define __DEPRECATED(msg) __attribute__((deprecated(msg)))
  #elif defined(__ICCARM__)
    #define __DEPRECATED(msg) _Pragma("deprecated")
  #else
    #define __DEPRECATED(msg)
  #endif
#endif

// 标记旧 API，并在 msg 中指明替换接口
__DEPRECATED("Please use hal_uart_init() instead.")
void hal_sys_uart_init(void);

__DEPRECATED("Please use hal_uart_write() instead.")
void hal_sys_putchar(char c);
```

### 阶段三：硬弃用与编译阻断 (Hard Deprecation & Build Break)
* **动作**：经过数周或一个子版本的缓冲后，通过预编译宏屏蔽旧接口的声明。
* **目的**：强制开发者停止使用老接口。若依然有遗留代码未改，编译将直接报错。只有主动在编译系统中开启类似 `-DSUPPORT_LEGACY_API` 的宏才能临时绕过，明确技术债。

**代码示例**：
```c
#ifdef SUPPORT_LEGACY_UART_API
    __DEPRECATED("Will be removed in next major release. Use hal_uart_write().")
    void hal_sys_putchar(char c);
#endif
```

### 阶段四：彻底移除 (Complete Removal)
* **动作**：在大版本更新（如 V3.0）时，将所有老接口的声明和兼容包装代码从代码库中彻底删除。
* **目的**：卸下历史包袱，保持系统干净整洁。

---

## 4. 落地建议
在当前的 SDK 维护阶段中，推荐**仅执行到【阶段一】和【阶段二】**：
1. 搭建起具备“端口/配置结构体”的新接口；
2. 将现有的业务功能改写为对新接口的包装；
3. 为老接口添加 `__DEPRECATED` 编译警告。

### 4.1 测试模板工程与日志规范
为了配合自动化与 AI 辅助分析，所有新增或未覆盖的外设 IP 测试模板工程（位于 `templates/`）必须：
1. **调用统一的 log 系统**：包含 `log.h` 并使用 `log_info()`, `log_debug()` 等接口替代直接的 `printf`。
2. **AI 友好化输出**：测试过程的关键节点（如 `[TEST_START]`, 读写寄存器、成功/失败结果）必须有清晰且结构化的日志输出，以便 AI 或自动化测试脚本能够准确解析测试状态。

这样，大家在写新业务代码时，看到编译器警告就会自觉使用新接口；而以前写好的老业务代码，虽然会报 Warning，但完全不用去动它们，保证了项目的稳定推进，也符合团队目前的资源现状。