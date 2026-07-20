#include "hal_timer.h"
#include "timer.h"

uint8_t hal_delay_us(uint8_t timer_id, uint32_t val) {
    (void)timer_id;
    delay_us(val);
    return 0;
}

uint8_t hal_delay_ms(uint8_t timer_id, uint32_t val) {
    (void)timer_id;
    delay_ms(val);
    return 0;
}

uint8_t hal_delay_s(uint8_t timer_id, uint32_t val) {
    (void)timer_id;
    delay_s(val);
    return 0;
}

uint8_t hal_sys_tick_init(uint8_t timer_id) {
    (void)timer_id;
    sys_tick_init();
    return 0;
}

uint32_t hal_get_sys_tick(uint8_t timer_id) {
    (void)timer_id;
    return get_sys_tick();
}
