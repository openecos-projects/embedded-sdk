#include "hal_rtc.h"
#include "board.h"

void hal_rtc_set_ctrl(uint32_t val) {
    REG_RTC_0_CTRL = val;
}

void hal_rtc_set_prescale(uint32_t val) {
    REG_RTC_0_PSCR = val - 1;
}

void hal_rtc_set_cnt(uint32_t val) {
    REG_RTC_0_CNT = val;
}

uint32_t hal_rtc_get_cnt(void) {
    return REG_RTC_0_CNT;
}

void hal_rtc_set_alrm(uint32_t val) {
    REG_RTC_0_ALRM = val;
}

uint32_t hal_rtc_get_ista(void) {
    return REG_RTC_0_ISTA;
}
