#include "main.h"
#include "hal_sys_uart.h"
#include "hal_rcu.h"
#include "ecos/log.h"
#include "hal_timer.h"
#include "hal_gpio.h"

void delay_ms(uint32_t val) {
    hal_delay_ms(0, val);
}

void rcu_test(){
  ECOS_LOGI("rcu", "[TEST_START] rcu_test");
    hal_rcu_set_ctrl(0b1011);
  hal_rcu_set_rdiv(256 - 1);
  ECOS_LOGI("rcu", "STAT: %d", hal_rcu_get_stat());
  }

int main() {
  hal_sys_uart_init();
  (void)ecos_log_set_level(ECOS_LOG_DEBUG);
  hal_sys_tick_init(0);
  rcu_test();
  while(1);
  return 0;
}
