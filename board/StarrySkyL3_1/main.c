#include "main.h"
#include "hal_sys_uart.h"
#include "hal_archinfo.h"
#include "hal_crc.h"
#include "hal_gpio.h"
#include "hal_ps2.h"
#include "hal_rcu.h"
#include "hal_rng.h"
#include "hal_rtc.h"
#include "hal_wdg.h"
#include "hal_timer.h"
#include "hal_pwm.h"
#include "hal_qspi.h"
#include "hal_i2c.h"
#include "st7735.h"
#include "pcf8563.h"
#include "logo.h"
#include <stdio.h>

void delay_ms(uint32_t val) {

    hal_timer_set_ctrl(HAL_TIMER_PORT_0, 0xD);
    for (int i = 0; i < val; ++i) {
        while (hal_timer_get_stat(HAL_TIMER_PORT_0) == 0);
    }
    hal_timer_set_ctrl(HAL_TIMER_PORT_0, 0xD);
}

void archinfo_test(){
  printf("=============================================\n");
  printf("             archinfo test                   \n");
  printf("=============================================\n");

  hal_archinfo_info();
  printf("write regs\n");
  hal_archinfo_set_sys(0x4004);
  hal_archinfo_set_idl(0x5F3E);
  hal_archinfo_set_idh(0x6E2);
  hal_archinfo_info();

  printf("=============================================\n\n");
}

void crc_test(){
  printf("==============================================\n");
  printf("                crc test                      \n");
  printf("==============================================\n");

  hal_crc_set_ctrl(0);
  hal_crc_set_init(0xFFFF);
  hal_crc_set_xorv(0);
  hal_crc_set_ctrl(0b1001001);
  uint32_t val = 0x123456;
  for(int i = 0; i < 10; ++i) { // Shortened to 10 for quick test
    hal_crc_set_data(val + i);
    // Add dummy delay/wait for STAT if applicable
    printf("i: %d CRC: %x\n", i, hal_crc_get_val());
  }

  printf("==============================================\n\n");
}

void gpio_led_test(){
  printf("==============================================\n");
  printf("              gpio led test                   \n");
  printf("==============================================\n");
  
  // Using GPIO0 Pin 2, 3, 4, 5 as output (example mask 0x3C)
  hal_gpio_output_enable(0, 2);
  hal_gpio_output_enable(0, 3);
  hal_gpio_output_enable(0, 4);
  hal_gpio_output_enable(0, 5);

  for(int i = 0; i<3; i++){ // Shortened to 3
    hal_gpio_set_level(0, 2, GPIO_LEVEL_HIGH);
    hal_gpio_set_level(0, 3, GPIO_LEVEL_HIGH);
    hal_gpio_set_level(0, 4, GPIO_LEVEL_HIGH);
    hal_gpio_set_level(0, 5, GPIO_LEVEL_HIGH);
    delay_ms(500);
    hal_gpio_set_level(0, 2, GPIO_LEVEL_LOW);
    hal_gpio_set_level(0, 3, GPIO_LEVEL_LOW);
    hal_gpio_set_level(0, 4, GPIO_LEVEL_LOW);
    hal_gpio_set_level(0, 5, GPIO_LEVEL_LOW);
    delay_ms(500);
  }

  printf("==============================================\n\n");
}

void i2c_pcf8563_test(){
  printf("==============================================\n");
  printf("            i2c pcf8563b test                 \n");
  printf("==============================================\n");

  hal_gpio_set_fcfg(0, 29, 1);
  hal_gpio_set_fcfg(0, 30, 1);
  hal_gpio_set_mux(0, 29, 0);
  hal_gpio_set_mux(0, 30, 0);

  hal_i2c_config_t i2c_cfg = { .pscr = 99 };
  hal_i2c_init(HAL_I2C_PORT_0, &i2c_cfg);

  pcf8563_device_t rtc_dev = {
      .i2c_port = HAL_I2C_PORT_0,
      .i2c_addr = 0xA2 // as original implementation
  };

  pcf8563_init(&rtc_dev);

  pcf8563_info_t init1_info = {.time.second = 00,
                                .time.minute = 33,
                                .time.hour = 14,
                                .date.weekday = 4,
                                .date.day = 15,
                                .date.month = 5,
                                .date.year = 25};
  pcf8563_write_info(&rtc_dev, &init1_info);

  printf("Write 2025 05 15 wednesday 14:33:00 into PCF8563B done\n");
  printf("read from PCF8563B\n");

  pcf8563_info_t rd_info = {0};

  for (int i = 0; i < 3; ++i) {
    rd_info = pcf8563_read_info(&rtc_dev);
    printf("[PCF8563B] 20%02d-%02d-%02d w:%d %02d:%02d:%02d\n", rd_info.date.year,
           rd_info.date.month, rd_info.date.day, rd_info.date.weekday,
           rd_info.time.hour, rd_info.time.minute, rd_info.time.second);
    delay_ms(1000);
  }

  pcf8563_info_t init2_info = {.time.second = 20,
                                .time.minute = 05,
                                .time.hour = 14,
                                .date.weekday = 3,
                                .date.day = 22,
                                .date.month = 5,
                                .date.year = 25};

  pcf8563_write_info(&rtc_dev, &init2_info);

  printf("Write 2025 05 22 wednesday 14:05:20 into PCF8563B done\n");
  printf("read from PCF8563B\n");

  for (int i = 0; i < 3; ++i) {
    rd_info = pcf8563_read_info(&rtc_dev);
    printf("[PCF8563B] 20%02d-%02d-%02d w:%d %02d:%02d:%02d\n", rd_info.date.year,
           rd_info.date.month, rd_info.date.day, rd_info.date.weekday,
           rd_info.time.hour, rd_info.time.minute, rd_info.time.second);
    delay_ms(1000);
  }

  printf("==============================================\n\n");
}

void ps2_test(){
  printf("==============================================\n");
  printf("              ps2 test                        \n");
  printf("==============================================\n");
  printf("ESC to exit\n");
  
  hal_gpio_set_fcfg(1, 12, 1);
  hal_gpio_set_fcfg(1, 13, 1);
  hal_gpio_set_mux(1, 12, 0); 
  hal_gpio_set_mux(1, 13, 0); 
  
  hal_ps2_set_ctrl(0b11);
  uint32_t kdb_code, i = 0;
  while (1) {
    kdb_code = hal_ps2_get_data();
    if (kdb_code != 0) {
      printf("[%d] dat: %x\n", i++, kdb_code);
    }
    if (kdb_code == 0x76) {
      break;
    }
  }
  printf("==============================================\n\n");
}

void pwm_led_test(){
  printf("==============================================\n");
  printf("              pwm led test                    \n");
  printf("==============================================\n");

  hal_gpio_set_fcfg(0, 2, 1);
  hal_gpio_set_fcfg(0, 3, 1);
  hal_gpio_set_fcfg(0, 4, 1);
  hal_gpio_set_fcfg(0, 5, 1);
  hal_gpio_set_mux(0, 2, 0);
  hal_gpio_set_mux(0, 3, 0);
  hal_gpio_set_mux(0, 4, 0);
  hal_gpio_set_mux(0, 5, 0);

  hal_pwm_set_prescale(HAL_PWM_PORT_0, 100);
  hal_pwm_set_cmp(HAL_PWM_PORT_0, 1000); 

  uint32_t cnt = 2; // Shortened
  while(cnt--){
    for (uint32_t duty = 0; duty <= 1000; duty += 100) {
      hal_pwm_set_ctrl(HAL_PWM_PORT_0, 4);
      hal_pwm_set_duty(HAL_PWM_PORT_0, 0, duty);
      hal_pwm_set_duty(HAL_PWM_PORT_0, 1, duty);
      hal_pwm_set_duty(HAL_PWM_PORT_0, 2, duty);
      hal_pwm_set_duty(HAL_PWM_PORT_0, 3, duty);
      hal_pwm_set_ctrl(HAL_PWM_PORT_0, 3);
      delay_ms(20);
    }
    for (uint32_t duty = 1000; duty >= 100; duty -= 100) {
      hal_pwm_set_ctrl(HAL_PWM_PORT_0, 4);
      hal_pwm_set_duty(HAL_PWM_PORT_0, 0, duty);
      hal_pwm_set_duty(HAL_PWM_PORT_0, 1, duty);
      hal_pwm_set_duty(HAL_PWM_PORT_0, 2, duty);
      hal_pwm_set_duty(HAL_PWM_PORT_0, 3, duty);
      hal_pwm_set_ctrl(HAL_PWM_PORT_0, 3);
      delay_ms(20);
    }
  }

  printf("==============================================\n\n");
}

void rcu_test(){
  printf("==============================================\n");
  printf("              rcu test                        \n");
  printf("==============================================\n");
  hal_rcu_set_ctrl(0b1011);
  hal_rcu_set_rdiv(256 - 1);
  printf("STAT: %d\n", hal_rcu_get_stat());
  printf("==============================================\n\n");
}

void rng_test(){
  printf("==============================================\n");
  printf("              rng test                        \n");
  printf("==============================================\n");
  hal_rng_set_ctrl(1);
  hal_rng_set_seed(0xFE1C);
  for(int i = 0; i < 5; ++i) {
      printf("[normal]random val: %x\n", hal_rng_get_val());
  }
  printf("reset the seed\n");
  hal_rng_set_seed(0);
  for(int i = 0; i < 3; ++i) {
      printf("[reset]zero val: %x\n", hal_rng_get_val());
  }
  printf("==============================================\n\n");
}

void rtc_test(){
  printf("==============================================\n");
  printf("              rtc test                        \n");
  printf("==============================================\n");

  hal_rtc_set_ctrl(1);
  hal_rtc_set_prescale(48); 
  
  for(uint32_t i = 0; i < 3; ++i) {
    hal_rtc_set_cnt(123 * i);
    hal_rtc_set_alrm(hal_rtc_get_cnt() + 10);
    printf("[static]CNT: %d\n", hal_rtc_get_cnt());
  }
  hal_rtc_set_cnt(0);
  hal_rtc_set_ctrl(0b0010010); // core and inc trg en
  
  printf("cnt inc test\n");
  for(int i = 0; i < 3; ++i) {
    while(hal_rtc_get_ista() != 1); 
    printf("RTC_REG_CNT: %d\n", hal_rtc_get_cnt());
  }
  printf("cnt inc test done\n");

  printf("==============================================\n\n");
}

void timer_test(){
  printf("==============================================\n");
  printf("              timer test                      \n"); 
  printf("==============================================\n");

  hal_timer_set_ctrl(HAL_TIMER_PORT_0, 0); // disable
  hal_timer_set_prescale(HAL_TIMER_PORT_0, 1 - 1);
  hal_timer_set_cmp(HAL_TIMER_PORT_0, 100000 - 1);
  
  printf("no div test start\n");
  for (int i = 1; i <= 3; ++i) {
    delay_ms(1000);
    printf("delay 1s\n");
  }
  printf("no div test done\n");

  printf("==============================================\n\n");
}

void wdg_test(){
  printf("==============================================\n");
  printf("              wdg test                        \n");
  printf("==============================================\n");

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
      printf("%d wdg reset trigger\n", i);
  }

  printf("==============================================\n\n");
}

/* ======================================================= */
/*                     ST7735 LCD Test                     */
/* ======================================================= */

st7735_device_t tft_dev = {
    .dc_gpio_port = 1,
    .dc_gpio_pin = 6,
    .qspi_port = HAL_QSPI_PORT_0,
    .qspi_cs = HAL_QSPI_CS_0,
    .screen_width = 128,
    .screen_height = 128,
    .rotation = 3, // based on original 0x68 configuration
    .horizontal_offset = 2,
    .vertical_offset = 3
};

void spi_tft_init() {
  printf("GPIO INIT:\n");
  hal_gpio_set_fcfg(1, 0, 1);
  hal_gpio_set_fcfg(1, 1, 1);
  hal_gpio_set_fcfg(1, 2, 1);
  hal_gpio_set_fcfg(1, 3, 1);
  hal_gpio_set_fcfg(1, 4, 1);
  hal_gpio_set_fcfg(1, 5, 1);
  
  hal_gpio_set_mux(1, 0, 0);
  hal_gpio_set_mux(1, 1, 0);
  hal_gpio_set_mux(1, 2, 0);
  hal_gpio_set_mux(1, 3, 0);
  hal_gpio_set_mux(1, 4, 0);
  hal_gpio_set_mux(1, 5, 0);
  
  printf("GPIO INIT DONE\n");
  
  hal_qspi_config_t qspi_config = { .clkdiv = 4 };
  hal_qspi_init(HAL_QSPI_PORT_0, &qspi_config);
  
  printf("tft init begin\n");
  st7735_init(&tft_dev);
}

void st7735_test(){
  printf("==============================================\n");
  printf("            st7735 lcd test                   \n");
  printf("==============================================\n");

  spi_tft_init();
  
  // Note: Since st7735_fill_img takes uint32_t array but gImage_CN is uint8_t, 
  // and the original local implementation used custom 8-bit copy,
  // we'll implement a local helper using the device driver
  st7735_addr_set(&tft_dev, 0, 0, 127, 127);
  for (uint16_t i = 0; i < 128 * 128 * 2; i++) {
    st7735_wr_data8(&tft_dev, gImage_CN[i]);
  }
  delay_ms(1000);

  st7735_addr_set(&tft_dev, 0, 0, 127, 127);
  for (uint16_t i = 0; i < 128 * 128 * 2; i++) {
    st7735_wr_data8(&tft_dev, gImage_YSYX[i]);
  }
  delay_ms(1000);

  printf("==============================================\n\n");
}

int main() {
  printf("StarrySkyL3_1 Smoke Test Start\n");

  hal_timer_set_prescale(HAL_TIMER_PORT_0, 1 - 1);
  hal_timer_set_cmp(HAL_TIMER_PORT_0, 100000 - 1);

  archinfo_test();
  crc_test();
  gpio_led_test();
  pwm_led_test();
  rcu_test();
  rng_test();
  rtc_test();
  timer_test();
  i2c_pcf8563_test();
  st7735_test();
  
  printf("NOTE: skipping PS2 test by default to prevent blocking.\n");
  printf("NOTE: skipping WDG test by default to prevent system reset.\n");

  // ps2_test();
  // wdg_test();

  printf("All test done\n");
  return 0;
}
