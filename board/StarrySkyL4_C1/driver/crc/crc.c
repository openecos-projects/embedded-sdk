#include "hal_crc.h"
#include "board.h"

void hal_crc_set_ctrl(uint32_t val) {
    REG_CRC_0_CTRL = val;
}

void hal_crc_set_init(uint32_t val) {
    REG_CRC_0_INIT = val;
}

void hal_crc_set_xorv(uint32_t val) {
    REG_CRC_0_XORV = val;
}

void hal_crc_set_data(uint32_t val) {
    REG_CRC_0_DATA = val;
}

uint32_t hal_crc_get_val(void) {
    return REG_CRC_0_DATA;
}
