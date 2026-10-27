/* CH32V103 device identification registers.
 * WCH documents 0x1FFFF884 as the 32-bit DBGMCU chip ID/revision word.
 * A separate 96-bit UID address is not published for this family, so the
 * public chip-id API returns this documented word and does not invent a UID.
 */
#include "ecos/hal/archinfo.h"

#include <stddef.h>
#include <stdint.h>

#define CH32V103_IDCODE (*(volatile uint32_t *)(uintptr_t)0x1FFFF884u)

static int valid_id(hal_archinfo_id_t id)
{
    return id == HAL_ARCHINFO_ID_0;
}

ecos_err_t hal_archinfo_get_system_id(hal_archinfo_id_t id, uint32_t *value)
{
    if (!valid_id(id) || value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    *value = CH32V103_IDCODE & 0x0000FFFFu;
    return ECOS_OK;
}

ecos_err_t hal_archinfo_get_chip_id(hal_archinfo_id_t id, uint64_t *value)
{
    if (!valid_id(id) || value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    *value = (uint64_t)CH32V103_IDCODE;
    return ECOS_OK;
}
