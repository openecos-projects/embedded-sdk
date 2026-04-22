#include "main.h"
#include "hal_sys_uart.h"
#include "hal_wdg.h"
#include "log.h"
#include "hal_timer.h"
#include "hal_gpio.h"

void delay_ms(uint32_t val) {
    hal_delay_ms(0, val);
}

void wdg_test(){
  log_info("[TEST_START] wdg_test");
  
  hal_wdg_set_key(0x5F3759DF);
  hal_wdg_set_ctrl(0x0);
  
  hal_wdg_set_key(0x5F3759DF);
  hal_wdg_set_prescale(100 - 1); 
  
  hal_wdg_set_key(0x5F3759DF);
  hal_wdg_set_cmp(500 - 1);      
  
  hal_wdg_set_key(0x5F3759DF);
  hal_wdg_set_ctrl(0b101);                    
  
  hal_wdg_set_key(0x5F3759DF);
  hal_wdg_feed(0x1);
  
  hal_wdg_set_key(0x5F3759DF);
  hal_wdg_feed(0x0);
  
  for(int i = 0; i < 3; ++i){
      while(hal_wdg_get_stat() == 0); 
      log_info("%d wdg reset trigger", i);
  }

  }

int main() {
  hal_sys_uart_init();
  log_init(LOG_DEBUG, NULL);
  hal_sys_tick_init(0);
  wdg_test();
  while(1);
  return 0;
}
