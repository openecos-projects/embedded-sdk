#ifndef __HAL_RTC_H__
#define __HAL_RTC_H__

#include <stdint.h>

void hal_rtc_set_ctrl(uint32_t val);
void hal_rtc_set_prescale(uint32_t val);
void hal_rtc_set_cnt(uint32_t val);
uint32_t hal_rtc_get_cnt(void);
void hal_rtc_set_alrm(uint32_t val);
uint32_t hal_rtc_get_ista(void);

#endif
