#include "ecos/heap.h"

#include <stddef.h>
#include <stdint.h>

/* K&R 风格首次适配空闲链表；单线程裸机模型，无锁、无 libc 依赖。 */

#define HEAP_MAX_POOLS      4u
#define HEAP_ALIGN          4u
#define HEAP_ALIGN_MASK     (HEAP_ALIGN - 1u)

typedef struct heap_block {
    struct heap_block *next;
    size_t size;    /* 整块字节数，含头部 */
} heap_block_t;

typedef struct {
    uintptr_t base;
    size_t size;
    ecos_heap_zone_t zone;
    heap_block_t *free_list;
} heap_pool_t;

static heap_pool_t g_pools[HEAP_MAX_POOLS];
static size_t g_pool_count;

static size_t heap_align_up(size_t value)
{
    return (value + HEAP_ALIGN_MASK) & ~(size_t)HEAP_ALIGN_MASK;
}

static heap_pool_t *heap_find_pool(void *ptr)
{
    uintptr_t header = (uintptr_t)ptr - sizeof(heap_block_t);
    size_t index;

    for (index = 0u; index < g_pool_count; ++index) {
        if (header >= g_pools[index].base &&
            header < g_pools[index].base + g_pools[index].size)
            return &g_pools[index];
    }
    return NULL;
}

static heap_block_t *heap_block_of(void *ptr)
{
    return (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
}

static void heap_copy_bytes(uint8_t *dest, const uint8_t *src, size_t size)
{
    size_t index;

    for (index = 0u; index < size; ++index)
        dest[index] = src[index];
}

ecos_err_t ecos_heap_add_pool(uintptr_t base, size_t size, ecos_heap_zone_t zone)
{
    heap_pool_t *pool;
    heap_block_t *initial;
    uintptr_t aligned_base = (uintptr_t)heap_align_up(base);

    if (zone != ECOS_HEAP_ZONE_INTERNAL && zone != ECOS_HEAP_ZONE_EXTERNAL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (aligned_base < base ||
        size < sizeof(heap_block_t) + HEAP_ALIGN + (aligned_base - base))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (g_pool_count >= HEAP_MAX_POOLS)
        return ECOS_ERR_NO_MEMORY;

    pool = &g_pools[g_pool_count];
    pool->base = aligned_base;
    pool->zone = zone;

    initial = (heap_block_t *)aligned_base;
    initial->next = NULL;
    initial->size = heap_align_up(size - (aligned_base - base));
    pool->size = initial->size;
    pool->free_list = initial;

    ++g_pool_count;
    return ECOS_OK;
}

void *ecos_heap_alloc(ecos_heap_zone_t zone, size_t size)
{
    size_t needed;
    size_t index;

    if (size == 0u)
        return NULL;

    needed = heap_align_up(size + sizeof(heap_block_t));

    for (index = 0u; index < g_pool_count; ++index) {
        heap_pool_t *pool = &g_pools[index];
        heap_block_t **link = &pool->free_list;

        if (pool->zone != zone)
            continue;

        while (*link != NULL) {
            heap_block_t *block = *link;

            if (block->size < needed) {
                link = &block->next;
                continue;
            }

            if (block->size >= needed + sizeof(heap_block_t) + HEAP_ALIGN) {
                /* 尾部足够大，拆出一个新空闲块留在链上。 */
                heap_block_t *remainder =
                    (heap_block_t *)((uint8_t *)block + needed);

                remainder->next = block->next;
                remainder->size = block->size - needed;
                *link = remainder;
                block->size = needed;
            } else {
                /* 剩余不足以成块，整块分配。 */
                *link = block->next;
            }

            block->next = NULL;
            return (uint8_t *)block + sizeof(heap_block_t);
        }
    }
    return NULL;
}

void *ecos_heap_malloc(size_t size)
{
    return ecos_heap_alloc(ECOS_HEAP_ZONE_INTERNAL, size);
}

void *ecos_heap_calloc(ecos_heap_zone_t zone, size_t count, size_t size)
{
    size_t total = count * size;
    uint8_t *ptr;

    if (count != 0u && total / count != size)
        return NULL;

    ptr = ecos_heap_alloc(zone, total);
    if (ptr != NULL) {
        size_t index;

        for (index = 0u; index < total; ++index)
            ptr[index] = 0u;
    }
    return ptr;
}

void *ecos_heap_realloc(void *ptr, size_t size)
{
    heap_block_t *block;
    heap_pool_t *pool;
    void *resized;
    size_t old_payload;
    size_t copy_size;

    if (ptr == NULL)
        return ecos_heap_malloc(size);
    if (size == 0u) {
        ecos_heap_free(ptr);
        return NULL;
    }

    pool = heap_find_pool(ptr);
    if (pool == NULL)
        return NULL;

    block = heap_block_of(ptr);
    old_payload = block->size - sizeof(heap_block_t);
    if (old_payload >= size)
        return ptr;

    resized = ecos_heap_alloc(pool->zone, size);
    if (resized == NULL)
        return NULL;

    copy_size = old_payload;
    heap_copy_bytes((uint8_t *)resized, (const uint8_t *)ptr, copy_size);
    ecos_heap_free(ptr);
    return resized;
}

void ecos_heap_free(void *ptr)
{
    heap_pool_t *pool;
    heap_block_t *block;
    heap_block_t *prev = NULL;
    heap_block_t *cursor;

    if (ptr == NULL)
        return;

    pool = heap_find_pool(ptr);
    if (pool == NULL)
        return;

    block = heap_block_of(ptr);

    /* 按地址有序插入空闲链。 */
    cursor = pool->free_list;
    while (cursor != NULL && cursor < block) {
        prev = cursor;
        cursor = cursor->next;
    }
    block->next = cursor;
    if (prev == NULL)
        pool->free_list = block;
    else
        prev->next = block;

    /* 与后继空闲块合并。 */
    if (cursor != NULL && (uint8_t *)block + block->size == (uint8_t *)cursor) {
        block->size += cursor->size;
        block->next = cursor->next;
    }
    /* 与前驱空闲块合并。 */
    if (prev != NULL && (uint8_t *)prev + prev->size == (uint8_t *)block) {
        prev->size += block->size;
        prev->next = block->next;
    }
}

size_t ecos_heap_free_bytes(ecos_heap_zone_t zone)
{
    size_t total = 0u;
    size_t index;

    for (index = 0u; index < g_pool_count; ++index) {
        heap_block_t *block;

        if (g_pools[index].zone != zone)
            continue;

        for (block = g_pools[index].free_list; block != NULL;
             block = block->next) {
            if (block->size > sizeof(heap_block_t))
                total += block->size - sizeof(heap_block_t);
        }
    }
    return total;
}
