#ifndef __HAL_TIMER_H__
#define __HAL_TIMER_H__

#include <stdint.h>

uint8_t hal_delay_us(uint8_t timer_id, uint32_t val);
uint8_t hal_delay_ms(uint8_t timer_id, uint32_t val);
uint8_t hal_delay_s(uint8_t timer_id, uint32_t val);
uint8_t hal_sys_tick_init(uint8_t timer_id);
uint32_t hal_get_sys_tick(uint8_t timer_id);

#endif