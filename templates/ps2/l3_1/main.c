#include "main.h"
#include "hal_sys_uart.h"
#include "hal_ps2.h"
#include "log.h"
#include "hal_timer.h"
#include "hal_gpio.h"

void delay_ms(uint32_t val) {
    hal_delay_ms(0, val);
}

void ps2_test(){
  log_info("[TEST_START] ps2_test");
    log_info("ESC to exit");
  
  gpio_hal_set_fcfg(1, 12, 1);
  gpio_hal_set_fcfg(1, 13, 1);
  gpio_hal_set_mux(1, 12, 0); 
  gpio_hal_set_mux(1, 13, 0); 
  
  hal_ps2_set_ctrl(0b11);
  uint32_t kdb_code, i = 0;
  while (1) {
    kdb_code = hal_ps2_get_data();
    if (kdb_code != 0) {
      log_info("[%d] dat: %x", i++, kdb_code);
    }
    if (kdb_code == 0x76) {
      break;
    }
  }
  }

int main() {
  hal_sys_uart_init();
  log_init(LOG_DEBUG, NULL);
  hal_sys_tick_init(0);
  ps2_test();
  while(1);
  return 0;
}
