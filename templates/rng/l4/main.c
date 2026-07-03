#include "main.h"
#include "hal_sys_uart.h"
#include "hal_rng.h"
#include "log.h"
#include "hal_timer.h"
#include "hal_gpio.h"

void rng_test(){
  log_info("[TEST_START] rng_test");
    hal_rng_set_ctrl(1);
  hal_rng_set_seed(0xFE1C);
  for(int i = 0; i < 5; ++i) {
      log_info("[normal]random val: %x", hal_rng_get_val());
  }
  log_info("reset the seed");
  hal_rng_set_seed(0);
  for(int i = 0; i < 3; ++i) {
      log_info("[reset]zero val: %x", hal_rng_get_val());
  }
  }

int main() {
  hal_sys_uart_init();
  log_init(LOG_DEBUG, NULL);
  hal_sys_tick_init(0);
  rng_test();
  while(1);
  return 0;
}
