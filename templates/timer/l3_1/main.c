#include "main.h"
#include "hal_sys_uart.h"
#include "hal_timer.h"
#include "ecos/log.h"
#include "hal_timer.h"
#include "hal_gpio.h"

void delay_ms(uint32_t val) {
    hal_delay_ms(0, val);
}

void timer_test(){
  ECOS_LOGI("timer", "[TEST_START] timer_test");
  ECOS_LOGI("timer", "==============================================");
  ECOS_LOGI("timer", "              timer test                      ");
  ECOS_LOGI("timer", "==============================================");

  hal_sys_tick_init(0);
  
  ECOS_LOGI("timer", "no div test start");
  for (int i = 1; i <= 3; ++i) {
    hal_delay_ms(0, 1000);
    ECOS_LOGI("timer", "delay 1s");
  }
  ECOS_LOGI("timer", "no div test done");

  }

int main() {
  hal_sys_uart_init();
  (void)ecos_log_set_level(ECOS_LOG_DEBUG);
  hal_sys_tick_init(0);
  timer_test();
  while(1);
  return 0;
}
