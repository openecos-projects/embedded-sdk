# Heap 动态分配组件

`heap` 是 SDK 3.0 的动态内存分配组件，提供双区堆模型：

- `ECOS_HEAP_ZONE_INTERNAL`：片上 SRAM 池，延迟低、启动即可用，
  `ecos_heap_malloc()` 的默认去向；
- `ECOS_HEAP_ZONE_EXTERNAL`：外部大容量内存池（如 SDRAM），适合显式申请的
  大缓冲（帧缓冲、算法工作集等）。

内存池由 Target 启动代码在 `main` 之前通过 `ecos_heap_add_pool()` 注册；
未注册任何池时分配返回 `NULL`。

## 接口

```c
ecos_err_t ecos_heap_add_pool(uintptr_t base, size_t size,
                              ecos_heap_zone_t zone);

void *ecos_heap_malloc(size_t size);                 /* INTERNAL 区 */
void *ecos_heap_alloc(ecos_heap_zone_t zone, size_t size);
void *ecos_heap_calloc(ecos_heap_zone_t zone, size_t count, size_t size);
void *ecos_heap_realloc(void *ptr, size_t size);
void ecos_heap_free(void *ptr);
size_t ecos_heap_free_bytes(ecos_heap_zone_t zone);
```

分配器是 K&R 风格首次适配空闲链表，4 字节对齐，`free` 时合并相邻空闲块。
SDK 为单线程裸机模型，接口无锁，不要在中断与主流程中并发调用。

## StarrySky T1-Pico

T1-Pico（cl1-2512）的启动代码在 `main` 前完成 SDRAM 初始化并注册两个池：
INTERNAL 为 DSRAM 堆区（`.data/.bss` 之后到 bank 末尾），EXTERNAL 为
32 MiB SDRAM。默认 `malloc` 落在 SRAM；大缓冲用
`ecos_heap_alloc(ECOS_HEAP_ZONE_EXTERNAL, size)` 显式申请。
