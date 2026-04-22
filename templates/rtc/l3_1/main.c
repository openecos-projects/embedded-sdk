#include "main.h"
#include "hal_sys_uart.h"
#include "hal_rtc.h"
#include "log.h"
#include "hal_timer.h"
#include "hal_gpio.h"

void delay_ms(uint32_t val) {
    hal_delay_ms(0, val);
}

void rtc_test(){
  log_info("[TEST_START] rtc_test");
  
  hal_rtc_set_ctrl(1);
  hal_rtc_set_prescale(48); 
  
  for(uint32_t i = 0; i < 3; ++i) {
    hal_rtc_set_cnt(123 * i);
    hal_rtc_set_alrm(hal_rtc_get_cnt() + 10);
    log_info("[static]CNT: %d", hal_rtc_get_cnt());
  }
  hal_rtc_set_cnt(0);
  hal_rtc_set_ctrl(0b0010010); // core and inc trg en
  
  log_info("cnt inc test");
  for(int i = 0; i < 3; ++i) {
    while(hal_rtc_get_ista() != 1); 
    log_info("RTC_REG_CNT: %d", hal_rtc_get_cnt());
  }
  log_info("cnt inc test done");

  }

int main() {
  hal_sys_uart_init();
  log_init(LOG_DEBUG, NULL);
  hal_sys_tick_init(0);
  rtc_test();
  while(1);
  return 0;
}
