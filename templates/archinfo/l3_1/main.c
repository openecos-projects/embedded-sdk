#include "main.h"
#include "hal_sys_uart.h"
#include "hal_archinfo.h"
#include "log.h"
#include "hal_timer.h"
#include "hal_gpio.h"

void archinfo_test(){
  log_info("[TEST_START] archinfo_test");
  
  hal_archinfo_info();
  log_info("write regs");
  hal_archinfo_set_sys(0x4004);
  hal_archinfo_set_idl(0x5F3E);
  hal_archinfo_set_idh(0x6E2);
  hal_archinfo_info();

  }

int main() {
  hal_sys_uart_init();
  log_init(LOG_DEBUG, NULL);
  hal_sys_tick_init(0);
  archinfo_test();
  while(1);
  return 0;
}
