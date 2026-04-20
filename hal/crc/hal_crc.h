#ifndef __HAL_CRC_H__
#define __HAL_CRC_H__

#include <stdint.h>

void hal_crc_set_ctrl(uint32_t val);
void hal_crc_set_init(uint32_t val);
void hal_crc_set_xorv(uint32_t val);
void hal_crc_set_data(uint32_t val);
uint32_t hal_crc_get_val(void);

#endif
