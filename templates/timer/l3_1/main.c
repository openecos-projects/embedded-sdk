#include "main.h"
#include "hal_sys_uart.h"
#include "hal_timer.h"
#include "log.h"
#include "hal_timer.h"
#include "hal_gpio.h"

void delay_ms(uint32_t val) {
    hal_delay_ms(0, val);
}

void timer_test(){
  log_info("[TEST_START] timer_test");
  log_info("==============================================");
  log_info("              timer test                      "); 
  log_info("==============================================");

  hal_sys_tick_init(0);
  
  log_info("no div test start");
  for (int i = 1; i <= 3; ++i) {
    hal_delay_ms(0, 1000);
    log_info("delay 1s");
  }
  log_info("no div test done");

  }

int main() {
  hal_sys_uart_init();
  log_init(LOG_DEBUG, NULL);
  hal_sys_tick_init(0);
  timer_test();
  while(1);
  return 0;
}
