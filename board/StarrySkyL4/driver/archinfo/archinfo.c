#include "hal_archinfo.h"
#include "board.h"
#include <stdio.h>

void hal_archinfo_info(void) {
    printf("SYS: %x IDL: %x IDH: %x\n", REG_ARCHINFO_0_SYS, REG_ARCHINFO_0_IDL, REG_ARCHINFO_0_IDH);
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
