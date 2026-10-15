#include "ecos/driver/clint.h"

#include "ecos/hal/clint.h"

#include <stddef.h>

static ecos_err_t clint_map_result(int result)
{
    if (result >= 0)
        return (ecos_err_t)result;
    return ecos_err_is_known(result) ? (ecos_err_t)result : ECOS_ERR_IO;
}

uint32_t ecos_clint_get_frequency_hz(void)
{
    return hal_clint_get_frequency_hz();
}

ecos_err_t ecos_clint_get_mtime(ecos_clint_value_t *value)
{
    hal_clint_value_t hal_value;
    ecos_err_t result;

    if (value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = clint_map_result(hal_clint_get_mtime(&hal_value));
    if (result == ECOS_OK) {
        value->low = hal_value.low;
        value->high = hal_value.high;
    }
    return result;
}

ecos_err_t ecos_clint_set_mtime(const ecos_clint_value_t *value)
{
    hal_clint_value_t hal_value;

    if (value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    hal_value.low = value->low;
    hal_value.high = value->high;
    return clint_map_result(hal_clint_set_mtime(&hal_value));
}

ecos_err_t ecos_clint_get_mtimecmp(ecos_clint_value_t *value)
{
    hal_clint_value_t hal_value;
    ecos_err_t result;

    if (value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    result = clint_map_result(hal_clint_get_mtimecmp(&hal_value));
    if (result == ECOS_OK) {
        value->low = hal_value.low;
        value->high = hal_value.high;
    }
    return result;
}

ecos_err_t ecos_clint_set_mtimecmp(const ecos_clint_value_t *value)
{
    hal_clint_value_t hal_value;

    if (value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    hal_value.low = value->low;
    hal_value.high = value->high;
    return clint_map_result(hal_clint_set_mtimecmp(&hal_value));
}
