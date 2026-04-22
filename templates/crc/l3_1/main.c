#include "main.h"
#include "hal_sys_uart.h"
#include "hal_crc.h"
#include "log.h"
#include "hal_timer.h"
#include "hal_gpio.h"

void delay_ms(uint32_t val) {
    hal_delay_ms(0, val);
}

void crc_test(){
  log_info("[TEST_START] crc_test");
  
  hal_crc_set_ctrl(0);
  hal_crc_set_init(0xFFFF);
  hal_crc_set_xorv(0);
  hal_crc_set_ctrl(0b1001001);
  uint32_t val = 0x123456;
  for(int i = 0; i < 10; ++i) { // Shortened to 10 for quick test
    hal_crc_set_data(val + i);
    // Add dummy delay/wait for STAT if applicable
    log_info("i: %d CRC: %x", i, hal_crc_get_val());
  }

  }

int main() {
  hal_sys_uart_init();
  log_init(LOG_DEBUG, NULL);
  hal_sys_tick_init(0);
  crc_test();
  while(1);
  return 0;
}
