#include "ecos/hal/clint.h"
#include "cl1_2512_soc.h"

#include <stddef.h>
#include <stdint.h>

ecos_err_t hal_clint_get_mtime(hal_clint_value_t *value)
{
    uint32_t high_first;
    uint32_t high_second;
    uint32_t low;

    if (value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    do {
        high_first = REG_CLINT_MTIME_HIGH;
        low = REG_CLINT_MTIME_LOW;
        high_second = REG_CLINT_MTIME_HIGH;
    } while (high_first != high_second);
    value->low = low;
    value->high = high_second;
    return ECOS_OK;
}

ecos_err_t hal_clint_set_mtime(const hal_clint_value_t *value)
{
    if (value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_CLINT_MTIME_LOW = 0u;
    REG_CLINT_MTIME_HIGH = value->high;
    REG_CLINT_MTIME_LOW = value->low;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return ECOS_OK;
}

ecos_err_t hal_clint_get_mtimecmp(hal_clint_value_t *value)
{
    if (value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    value->high = REG_CLINT_MTIMECMP_HIGH;
    value->low = REG_CLINT_MTIMECMP_LOW;
    return ECOS_OK;
}

ecos_err_t hal_clint_set_mtimecmp(const hal_clint_value_t *value)
{
    if (value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    REG_CLINT_MTIMECMP_HIGH = UINT32_MAX;
    REG_CLINT_MTIMECMP_LOW = value->low;
    REG_CLINT_MTIMECMP_HIGH = value->high;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return ECOS_OK;
}

uint32_t hal_clint_get_frequency_hz(void)
{
    return CL1_2512_CLINT_MTIME_FREQ_HZ;
}
