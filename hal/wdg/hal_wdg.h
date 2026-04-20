#ifndef __HAL_WDG_H__
#define __HAL_WDG_H__

#include <stdint.h>

void hal_wdg_set_ctrl(uint32_t val);
void hal_wdg_set_prescale(uint32_t val);
void hal_wdg_set_cmp(uint32_t val);
void hal_wdg_feed(uint32_t val);
void hal_wdg_set_key(uint32_t val);
uint32_t hal_wdg_get_stat(void);

#endif
