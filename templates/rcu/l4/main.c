#include "main.h"
#include "hal_sys_uart.h"
#include "hal_rcu.h"
#include "log.h"
#include "hal_timer.h"
#include "hal_gpio.h"

void delay_ms(uint32_t val) {
    hal_delay_ms(0, val);
}

void rcu_test(){
  log_info("[TEST_START] rcu_test");
    hal_rcu_set_ctrl(0b1011);
  hal_rcu_set_rdiv(256 - 1);
  log_info("STAT: %d", hal_rcu_get_stat());
  }

int main() {
  hal_sys_uart_init();
  log_init(LOG_DEBUG, NULL);
  hal_sys_tick_init(0);
  rcu_test();
  while(1);
  return 0;
}
