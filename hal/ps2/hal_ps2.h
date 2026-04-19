#ifndef __HAL_PS2_H__
#define __HAL_PS2_H__

#include <stdint.h>

void hal_ps2_set_ctrl(uint32_t val);
uint32_t hal_ps2_get_data(void);

#endif
