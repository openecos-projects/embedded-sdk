#include <am.h>


/**
 * 初始化 L4 单核 MPE。
 */
bool mpe_init(void (*entry)())
{
    /* 单核模式直接执行入口，不依赖 A 扩展。 */
    if (entry != NULL)
        entry();

    return true;
}


/**
 * 返回可用 CPU 数量。
 */
int cpu_count()
{
    /* L4 当前只开放单核运行。 */
    return 1;
}


/**
 * 返回当前 CPU 编号。
 */
int cpu_current()
{
    /* 单核模式固定返回 CPU0。 */
    return 0;
}


/**
 * 执行单核软件原子交换。
 */
int atomic_xchg(int *addr, int newval)
{
    int oldval;

    /* base profile 没有中断和 A 扩展，单核交换无需 CSR。 */
    oldval = *addr;
    *addr = newval;
    return oldval;
}
