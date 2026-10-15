#include "hal_wdg.h"
#include "board.h"

void hal_wdg_set_ctrl(uint32_t val) {
    REG_WDG_0_CTRL = val;
}

void hal_wdg_set_prescale(uint32_t val) {
    REG_WDG_0_PSCR = val;
}

void hal_wdg_set_cmp(uint32_t val) {
    REG_WDG_0_CMP = val;
}

void hal_wdg_feed(uint32_t val) {
    REG_WDG_0_FEED = val;
}

void hal_wdg_set_key(uint32_t val) {
    REG_WDG_0_KEY = val;
}

uint32_t hal_wdg_get_stat(void) {
    return REG_WDG_0_STAT;
}
