#include <am.h>


/**
 * 报告 L4 当前不支持虚拟内存。
 */
bool vme_init(void *(*pgalloc_f)(int), void (*pgfree_f)(void *))
{
    /* 保留参数接口但不建立页表契约。 */
    (void)pgalloc_f;
    (void)pgfree_f;
    return false;
}


/**
 * 保持未启用的地址空间为空。
 */
void protect(AddrSpace *as)
{
    /* L4 当前没有用户态地址空间。 */
    (void)as;
}


/**
 * 释放未启用的地址空间。
 */
void unprotect(AddrSpace *as)
{
    /* L4 当前没有需要释放的页表。 */
    (void)as;
}


/**
 * 忽略未启用地址空间的映射请求。
 */
void map(AddrSpace *as, void *va, void *pa, int prot)
{
    /* 明确消费参数，避免产生未使用告警。 */
    (void)as;
    (void)va;
    (void)pa;
    (void)prot;
}


/**
 * 拒绝创建用户态 Context。
 */
Context *ucontext(AddrSpace *as, Area kstack, void *entry)
{
    /* 未建立 VME 契约时返回空 Context。 */
    (void)as;
    (void)kstack;
    (void)entry;
    return NULL;
}
