#include "hal_ps2.h"
#include "board.h"

void hal_ps2_set_ctrl(uint32_t val) {
    REG_PS2_0_CTRL = val;
}

uint32_t hal_ps2_get_data(void) {
    return REG_PS2_0_DATA;
}
