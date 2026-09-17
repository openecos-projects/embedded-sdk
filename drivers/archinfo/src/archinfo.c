#include "ecos/driver/archinfo.h"

#include "ecos/hal/archinfo.h"

#include <stddef.h>
#include <stdint.h>

static int archinfo_map_hal_result(int result)
{
    if (result >= 0)
        return result;
    return ecos_err_is_known(result) ? result : ECOS_ERR_IO;
}

ecos_err_t ecos_archinfo_get_system_id(ecos_archinfo_id_t id, uint32_t *value)
{
    if (id >= ECOS_ARCHINFO_ID_COUNT || value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    return archinfo_map_hal_result(
        hal_archinfo_get_system_id((hal_archinfo_id_t)id, value)
    );
}

ecos_err_t ecos_archinfo_get_chip_id(ecos_archinfo_id_t id, uint64_t *value)
{
    if (id >= ECOS_ARCHINFO_ID_COUNT || value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    return archinfo_map_hal_result(
        hal_archinfo_get_chip_id((hal_archinfo_id_t)id, value)
    );
}
