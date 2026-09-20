#ifndef ECOS_HEAP_H
#define ECOS_HEAP_H

#include "ecos/error.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* INTERNAL：SoC 片上 SRAM，延迟低、启动即可用，默认分配去向；
 * EXTERNAL：外部大容量内存（如 SDRAM），容量大，适合显式申请的大缓冲。 */
typedef enum {
    ECOS_HEAP_ZONE_INTERNAL = 0,
    ECOS_HEAP_ZONE_EXTERNAL
} ecos_heap_zone_t;

/* 注册一个内存池；一般由 Target 启动代码在 main 之前调用。 */
ecos_err_t ecos_heap_add_pool(uintptr_t base, size_t size, ecos_heap_zone_t zone);

void *ecos_heap_malloc(size_t size);
void *ecos_heap_alloc(ecos_heap_zone_t zone, size_t size);
void *ecos_heap_calloc(ecos_heap_zone_t zone, size_t count, size_t size);

/* 在原 zone 内搬迁式 realloc（新尺寸为 0 时等价 free）。 */
void *ecos_heap_realloc(void *ptr, size_t size);

void ecos_heap_free(void *ptr);

/* 该 zone 当前可分配字节数（含碎片，单次最大分配可能更小）。 */
size_t ecos_heap_free_bytes(ecos_heap_zone_t zone);

#ifdef __cplusplus
}
#endif

#endif
