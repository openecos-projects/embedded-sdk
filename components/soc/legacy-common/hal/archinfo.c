#include "ecos/hal/archinfo.h"
#include "legacy_soc.h"

#include <stddef.h>

ecos_err_t hal_archinfo_get_system_id(hal_archinfo_id_t id, uint32_t *value)
{
    if (id != HAL_ARCHINFO_ID_0 || value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    *value = REG_ARCHINFO_0_SYS;
    return ECOS_OK;
}

ecos_err_t hal_archinfo_get_chip_id(hal_archinfo_id_t id, uint64_t *value)
{
    if (id != HAL_ARCHINFO_ID_0 || value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    *value = ((uint64_t)REG_ARCHINFO_0_IDH << 32) | REG_ARCHINFO_0_IDL;
    return ECOS_OK;
}
