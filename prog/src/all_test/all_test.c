#include "ysyx.h"
#include <am.h>
#include <klib-macros.h>
#include <klib.h>
#include "logo.h"

void archinfo_test(){
  printf("=============================================\n");
  printf("             archinfo test                   \n");
  printf("=============================================\n");

  ARCH_Info();
  putstr("write regs\n");
  ARCH_SetSys((uint32_t)0x4004);
  ARCH_SetIdl((uint32_t)0x5F3E);
  ARCH_SetIdh((uint32_t)0x6E2);
  ARCH_Info();

  printf("=============================================\n\n");
}

void crc_test(){
  printf("==============================================\n");
  printf("                crc test                      \n");
  printf("==============================================\n");

  CRC_SetCtrl(0);
  CRC_SetInit(0xFFFF);
  CRC_SetXorV(0);
  CRC_SetCtrl(0b1001001);
  uint32_t val = 0x123456;
  for(int i = 0; i < 50; ++i) {
    CRC_SetData(val + i);
    while(CRC_REG_STAT == (uint32_t)0);
    printf("i: %d CRC: %x\n", i, CRC_GetVal());
  }

  printf("==============================================\n\n");
}

void gpio_led_test(){
  printf("==============================================\n");
  printf("              gpio led test                   \n");
  printf("==============================================\n");
  
  GPIO_SetDir(GPIO_0, 0x3C);
  for(int i = 0; i<10; i++){
    GPIO_SetVal(GPIO_0, 0x003C);
    delay_ms(500);
    GPIO_SetVal(GPIO_0, 0x0000);
    delay_ms(500);
  }

  printf("==============================================\n\n");
}

#define PCF8563B_SLV_ADDR 0xA2

#define PCF8563B_CTL_STATUS1 ((uint8_t)0x00)
#define PCF8563B_CTL_STATUS2 ((uint8_t)0x01)
#define PCF8563B_SECOND_REG ((uint8_t)0x02)
#define PCF8563B_MINUTE_REG ((uint8_t)0x03)
#define PCF8563B_HOUR_REG ((uint8_t)0x04)
#define PCF8563B_DAY_REG ((uint8_t)0x05)
#define PCF8563B_WEEKDAY_REG ((uint8_t)0x06)
#define PCF8563B_MONTH_REG ((uint8_t)0x07)
#define PCF8563B_YEAR_REG ((uint8_t)0x08)

#define SECOND_MINUTE_REG_WIDTH ((uint8_t)0x7F)
#define HOUR_DAY_REG_WIDTH ((uint8_t)0x3F)
#define WEEKDAY_REG_WIDTH ((uint8_t)0x07)
#define MONTH_REG_WIDTH ((uint8_t)0x1F)
#define YEAR_REG_WIDTH ((uint8_t)0xFF)
#define BCD_Century ((uint8_t)0x80)

typedef struct {
  struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
  } time;

  struct {
    uint8_t weekday;
    uint8_t day;
    uint8_t month;
    uint8_t year;
  } date;

} PCF8563B_info_t;


uint8_t PCF8563B_bin2bcd(uint8_t val) {
  uint8_t bcdhigh = 0;
  while (val >= 10) {
    ++bcdhigh;
    val -= 10;
  }
  return ((uint8_t)(bcdhigh << 4) | val);
}

static uint8_t PCF8563B_bcd2bin(uint8_t val, uint8_t reg_width) {
  uint8_t res = 0;
  res = (val & (reg_width & 0xF0)) >> 4;
  res = res * 10 + (val & (reg_width & 0x0F));
  return res;
}

void PCF8563B_wr_reg(PCF8563B_info_t *info) {
  uint8_t wr_data[7] = {0};
  *wr_data = PCF8563B_bin2bcd(info->time.second);
  *(wr_data + 1) = PCF8563B_bin2bcd(info->time.minute);
  *(wr_data + 2) = PCF8563B_bin2bcd(info->time.hour);
  *(wr_data + 3) = PCF8563B_bin2bcd(info->date.day);
  *(wr_data + 4) = PCF8563B_bin2bcd(info->date.weekday);
  *(wr_data + 5) = PCF8563B_bin2bcd(info->date.month);
  *(wr_data + 6) = PCF8563B_bin2bcd(info->date.year);
  i2c_wr_nbyte(PCF8563B_SLV_ADDR, PCF8563B_SECOND_REG, I2C_DEV_ADDR_8BIT, 7,
               wr_data);
}

PCF8563B_info_t PCF8563B_rd_reg() {
  uint8_t rd_data[7] = {0};
  PCF8563B_info_t info = {0};
  i2c_rd_nbyte(PCF8563B_SLV_ADDR, PCF8563B_SECOND_REG, I2C_DEV_ADDR_8BIT, 7,
               rd_data);
  info.time.second = PCF8563B_bcd2bin(rd_data[0], SECOND_MINUTE_REG_WIDTH);
  info.time.minute = PCF8563B_bcd2bin(rd_data[1], SECOND_MINUTE_REG_WIDTH);
  info.time.hour = PCF8563B_bcd2bin(rd_data[2], HOUR_DAY_REG_WIDTH);
  info.date.day = PCF8563B_bcd2bin(rd_data[3], HOUR_DAY_REG_WIDTH);
  info.date.weekday = PCF8563B_bcd2bin(rd_data[4], WEEKDAY_REG_WIDTH);
  info.date.month = PCF8563B_bcd2bin(rd_data[5], MONTH_REG_WIDTH);
  info.date.year = PCF8563B_bcd2bin(rd_data[6], YEAR_REG_WIDTH);
  return info;
}

void i2c_pcf8563b_test(){
  printf("==============================================\n");
  printf("            i2c pcf8563b test                 \n");
  printf("==============================================\n");

  i2c_config();

  PCF8563B_info_t init1_info = {.time.second = 00,
                                .time.minute = 33,
                                .time.hour = 14,
                                .date.weekday = 4,
                                .date.day = 15,
                                .date.month = 5,
                                .date.year = 25};
  PCF8563B_wr_reg(&init1_info);

  printf("Write 2025 05 15 wednesday 14:33:00 into PCF8563B done\n");
  printf("read from PCF8563B\n");

  PCF8563B_info_t rd_info = {0};

  for (int i = 0; i < 10; ++i) {
    rd_info = PCF8563B_rd_reg();
    printf("\033[34m[PCF8563B] %d-%d-%d %d %d:%d:%d\n\033[0m", rd_info.date.year,
           rd_info.date.month, rd_info.date.day, rd_info.date.weekday,
           rd_info.time.hour, rd_info.time.minute, rd_info.time.second);
    delay_ms(1000);
  }

  PCF8563B_info_t init2_info = {.time.second = 20,
                                .time.minute = 05,
                                .time.hour = 14,
                                .date.weekday = 3,
                                .date.day = 22,
                                .date.month = 5,
                                .date.year = 25};

  PCF8563B_wr_reg(&init2_info);

  printf("Write 2025 05 22 wednesday 14:05:20 into PCF8563B done\n");
  printf("read from PCF8563B\n");

  for (int i = 0; i < 10; ++i) {
    rd_info = PCF8563B_rd_reg();
    printf("\033[34m[PCF8563B] %d-%d-%d %d %d:%d:%d\n\033[0m", rd_info.date.year,
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
  GPIO_SetFCFG_Num(GPIO_1, 12, 1);
  GPIO_SetFCFG_Num(GPIO_1, 13, 1);
  GPIO_SetMUX(GPIO_1, 0x0); // [12, 13] = 0
  PS2_SetCtrl((uint32_t)0b11);
  uint32_t kdb_code, i = 0;
  while (1) {
    kdb_code = PS2_GetData();
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

  GPIO_SetFCFG(GPIO_0, (uint32_t)0xF << 2);
  GPIO_SetMUX(GPIO_0, 0x00);
  PWM_Init(PWM_0, 100, 1000); // 1KHz

  uint32_t cnt = 5;
  while(cnt--){
    for (uint32_t duty = 0; duty <= 1000; duty += 50) {
      PWM_SetCtr(PWM_0, 4);
      PWM_SetDuty(PWM_0, PWM_CHANAL_0, duty);
      PWM_SetDuty(PWM_0, PWM_CHANAL_1, duty);
      PWM_SetDuty(PWM_0, PWM_CHANAL_2, duty);
      PWM_SetDuty(PWM_0, PWM_CHANAL_3, duty);
      PWM_SetCtr(PWM_0, 3);
      delay_ms(20);
    }
    for (uint32_t duty = 1000; duty >= 50; duty -= 50) {
      PWM_SetCtr(PWM_0, 4);
      PWM_SetDuty(PWM_0, PWM_CHANAL_0, duty);
      PWM_SetDuty(PWM_0, PWM_CHANAL_1, duty);
      PWM_SetDuty(PWM_0, PWM_CHANAL_2, duty);
      PWM_SetDuty(PWM_0, PWM_CHANAL_3, duty);
      PWM_SetCtr(PWM_0, 3);
      delay_ms(20);
    }
  }

  printf("==============================================\n\n");
}

void rcu_test(){
  printf("==============================================\n");
  printf("              rcu test                        \n");
  printf("==============================================\n");
  RCU_SetCtrl(0b1011);
  RCU_SetRdiv(256 - 1);
  printf("CTRL: %d RDIV: %d CLKLOCK: %d\n", RCU_REG_CTRL, RCU_REG_RDIV, RCU_REG_STAT);
  printf("==============================================\n\n");
}

void rng_test(){
  printf("==============================================\n");
  printf("              rng test                        \n");
  printf("==============================================\n");
  RNG_SetCtrl(1);
  RNG_SetSeed(0xFE1C);
  for(int i = 0; i < 10; ++i) {
      printf("[normal]random val: %x\n", RNG_GetVal());
  }
  putstr("reset the seed\n");
  RNG_SetSeed(0);
  for(int i = 0; i < 3; ++i) {
      printf("[reset]zero val: %x\n", RNG_GetVal());
  }
  printf("==============================================\n\n");
}

void rtc_test(){
  printf("==============================================\n");
  printf("              rtc test                        \n");
  printf("==============================================\n");

  RTC_SetCtrl(1);
  RTC_SetPrescale(48); 
  printf("RCU_REG_CTRL: %d\n", RCU_REG_CTRL);
  printf("RCU_REG_RDIV: %d\n", RCU_REG_RDIV);
  for(uint32_t i = 0; i < 6; ++i) {
    RTC_SetCnt(123 * i);
    RTC_SetAlrm(RTC_GetCnt() + 10);
    printf("[static]CNT: %d ALRM: %d\n", RTC_GetCnt(), RTC_REG_ALRM);
    if(RTC_GetCnt() != (uint32_t)(123 * i)) {
      putstr("error\n");
      printf("[error]CNT: %d ALRM: %d I:=%d\n", RTC_GetCnt(), RTC_REG_ALRM, i);
    }
  }
  RTC_SetCnt(0);
  RTC_SetCtrl(0b0010010); // core and inc trg en
  printf("CTRL: %d PSCR: %d\n", RTC_REG_CTRL, RTC_REG_PSCR);
  
  putstr("cnt inc test\n");
  for(int i = 0; i < 6; ++i) {
    while(RTC_GetIsta() != (uint32_t)1); // wait inc irq flag
    printf("RTC_REG_CNT: %d\n", RTC_GetCnt()); // inc 1 in 1/6s
  }
  putstr("cnt inc test done\n");
  
  putstr("alrm trigger test\n");
  RTC_SetCtrl(1); // enter config mode
  RTC_SetCnt(0);
  RTC_SetAlrm(RTC_GetCnt() + 6);
  for(uint32_t i=0; i<6; ++i) {
    RTC_SetCtrl(0b0010100);
    while(RTC_GetIsta() != (uint32_t)2);       // wait alrm irq flag
    RTC_SetCtrl(1);               // enter config mode
    while(RTC_GetIsta() != (uint32_t)0);       // clear the all irq flag
    printf("RTC_REG_CNT: %d\n", RTC_GetCnt()); // alrm trg every 1s
    RTC_SetAlrm(RTC_GetCnt() + 6);
  }
  printf("CTRL: %d PSCR: %d\n", RTC_REG_CTRL, RTC_REG_PSCR);
  putstr("alrm trigger test done\n");

  printf("==============================================\n\n");
}

void st7735_test(){
  printf("==============================================\n");
  printf("            st7735 lcd test                   \n");
  printf("==============================================\n");

  spi_tft_init();
  
  lcd_fill_bmp(gImage_CN, 0, 0, 128, 128);
  delay_ms(1000);
  lcd_fill_bmp(gImage_YSYX, 0, 0, 128, 128);
  delay_ms(1000);

  printf("==============================================\n\n");
}

void timer_test(){
  printf("==============================================\n");
  printf("              timer test                      \n"); 
  printf("==============================================\n");

  timer_init(1, 100000);
  putstr("no div test start\n");
  for (int i = 1; i <= 10; ++i) {
    delay_ms(1000);
    putstr("delay 1s\n");
  }
  putstr("no div test done\n");

  putstr("div test start\n");
  // 100MHz for 1s
  timer_init(100, 1000000);
  // timer_init(100, 10); // for soc
  for (int i = 1; i <= 10; ++i) {
    delay_ms(1);
    putstr("delay 1s\n");
  }

  printf("==============================================\n\n");
  timer_init(1, 100000);

}

void wdg_test(){
  printf("==============================================\n");
  printf("              wdg test                        \n");
  printf("==============================================\n");

  WDG_SetKey(WDG_MAGIC_NUM);
  WDG_SetCtrl(0x0);
  // feed wdg in every 50ms
  WDG_SetKey(WDG_MAGIC_NUM);
  WDG_SetPrescale((uint32_t)(100 - 1)); // div 100 for 1MHz(100MHz)   
  WDG_SetKey(WDG_MAGIC_NUM);
  WDG_SetCmp((uint32_t)(500 - 1));      // overflow in every 500ns    
  while (WDG_GetStat() == (uint32_t)0x1); // clear irq flag
  WDG_SetKey(WDG_MAGIC_NUM);
  WDG_SetCtrl(0b101);                    // core and ov trg en
  WDG_SetKey(WDG_MAGIC_NUM);
  WDG_Feed(0x1);
  WDG_SetKey(WDG_MAGIC_NUM);
  WDG_Feed(0x0);
  for(int i = 0; i < 10; ++i){
      printf("WDG_REG_PSCR: %d\n", WDG_REG_PSCR);
      while(WDG_GetStat() == (uint32_t)0); // wait for
      printf("%d wdg reset trigger\n", i);
  }

  printf("==============================================\n\n");
}
int main() {
  printf("All test start\n");

  timer_init(1, 100000);
  archinfo_test();
  crc_test();
  gpio_led_test();
  i2c_pcf8563b_test();
  ps2_test();

  pwm_led_test();
  rcu_test();
  rng_test();
  rtc_test();
  st7735_test();
  timer_test();
  wdg_test();

  putstr("All test done\n");
  return 0;
}
