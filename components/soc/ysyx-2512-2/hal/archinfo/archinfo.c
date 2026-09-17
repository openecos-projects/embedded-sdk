#include "ecos/hal/archinfo.h"
#include "hal_archinfo.h"
#include "ysyx_2512_2_soc.h"

#include <stdio.h>

static int archinfo_id_is_valid(hal_archinfo_id_t id)
{
    return id == HAL_ARCHINFO_ID_0;
}

ecos_err_t hal_archinfo_get_system_id(hal_archinfo_id_t id, uint32_t *value)
{
    if (!archinfo_id_is_valid(id) || value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    *value = REG_ARCHINFO_0_SYS;
    return ECOS_OK;
}

ecos_err_t hal_archinfo_get_chip_id(hal_archinfo_id_t id, uint64_t *value)
{
    if (!archinfo_id_is_valid(id) || value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;

    *value = ((uint64_t)REG_ARCHINFO_0_IDH << 32) | REG_ARCHINFO_0_IDL;
    return ECOS_OK;
}

/* ---- SDK 2.x compatibility API ---- */

void hal_archinfo_info(void) {
    printf("SYS: %x IDL: %x IDH: %x\n",
           (unsigned int)REG_ARCHINFO_0_SYS,
           (unsigned int)REG_ARCHINFO_0_IDL,
           (unsigned int)REG_ARCHINFO_0_IDH);
}

void hal_archinfo_set_sys(uint32_t val) {
    REG_ARCHINFO_0_SYS = val;
}

void hal_archinfo_set_idl(uint32_t val) {
    REG_ARCHINFO_0_IDL = val;
}

void hal_archinfo_set_idh(uint32_t val) {
    REG_ARCHINFO_0_IDH = val;
}

uint32_t hal_archinfo_get_sys(void) {
    return REG_ARCHINFO_0_SYS;
}

uint32_t hal_archinfo_get_idl(void) {
    return REG_ARCHINFO_0_IDL;
}

uint32_t hal_archinfo_get_idh(void) {
    return REG_ARCHINFO_0_IDH;
}
