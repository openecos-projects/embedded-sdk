#include <stddef.h>
#include <stdint.h>

#include "hal_clint.h"
#include "board.h"

/**
 * 一致读取 CLINT mtime 的高低三十二位。
 */
int hal_clint_get_mtime(hal_clint_value_t *value)
{
    uint32_t high_first;
    uint32_t high_second;
    uint32_t low;

    /* 拒绝空输出指针。 */
    if (value == NULL)
        return -1;

    /* 重读高位直到低位采样期间未发生高位进位。 */
    do
    {
        high_first = REG_CLINT_MTIME_HIGH;
        low = REG_CLINT_MTIME_LOW;
        high_second = REG_CLINT_MTIME_HIGH;
    } while (high_first != high_second);

    /* 返回同一稳定计数快照的高低位。 */
    value->low = low;
    value->high = high_second;
    return 0;
}


/**
 * 写入 CLINT mtime 的高低三十二位。
 */
int hal_clint_set_mtime(const hal_clint_value_t *value)
{
    /* 拒绝空输入指针。 */
    if (value == NULL)
        return -1;

    /* 先清低位再写高位和最终低位，避免旧低位产生进位。 */
    REG_CLINT_MTIME_LOW = 0u;
    REG_CLINT_MTIME_HIGH = value->high;
    REG_CLINT_MTIME_LOW = value->low;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return 0;
}


/**
 * 读取 CLINT mtimecmp 的高低三十二位。
 */
int hal_clint_get_mtimecmp(hal_clint_value_t *value)
{
    /* 拒绝空输出指针。 */
    if (value == NULL)
        return -1;

    /* 比较寄存器仅由软件更新，可以直接读取高低位。 */
    value->high = REG_CLINT_MTIMECMP_HIGH;
    value->low = REG_CLINT_MTIMECMP_LOW;
    return 0;
}


/**
 * 安全写入 CLINT mtimecmp 的高低三十二位。
 */
int hal_clint_set_mtimecmp(const hal_clint_value_t *value)
{
    /* 拒绝空输入指针。 */
    if (value == NULL)
        return -1;

    /* 先将高位设为最大值，避免低位更新期间产生瞬时比较中断。 */
    REG_CLINT_MTIMECMP_HIGH = UINT32_MAX;
    REG_CLINT_MTIMECMP_LOW = value->low;
    REG_CLINT_MTIMECMP_HIGH = value->high;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return 0;
}
