#include "ecos/driver/ps2.h"

#include "ecos/hal/ps2.h"

#include <stddef.h>

static int ps2_map_hal_result(int result)
{
    if (result >= 0)
        return result;
    return ecos_err_is_known(result) ? result : ECOS_ERR_IO;
}

ecos_err_t ecos_ps2_init(ecos_ps2_id_t id)
{
    if (id >= ECOS_PS2_ID_COUNT)
        return ECOS_ERR_INVALID_ARGUMENT;
    return ps2_map_hal_result(hal_ps2_init((hal_ps2_id_t)id));
}

ecos_err_t ecos_ps2_deinit(ecos_ps2_id_t id)
{
    if (id >= ECOS_PS2_ID_COUNT)
        return ECOS_ERR_INVALID_ARGUMENT;
    return ps2_map_hal_result(hal_ps2_deinit((hal_ps2_id_t)id));
}

ecos_err_t ecos_ps2_read(ecos_ps2_id_t id, uint32_t *scan_code)
{
    if (id >= ECOS_PS2_ID_COUNT || scan_code == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    return ps2_map_hal_result(hal_ps2_read((hal_ps2_id_t)id, scan_code));
}
