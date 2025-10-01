#include "am.h"
#include "klib-macros.h"
#include "klib.h"
#include "stdint.h"
#include <stdint.h>
#include <stdio.h>
#include "../include/ysyx.h"

void info_id() {
  uint32_t mvendorid;
  __asm__ volatile("csrr %0, mvendorid" : "=r"(mvendorid));
  uint64_t marchid;
  __asm__ volatile("csrr %0, marchid" : "=r"(marchid));
  uint64_t mimpid;
  __asm__ volatile("csrr %0, mimpid" : "=r"(mimpid));

  char mvendorid_ch[5];
  for (int i = 0; i < 4; i++) {
    mvendorid_ch[i] = (mvendorid >> (i * 8)) & 0xFF;
  }
  mvendorid_ch[4] = '\0';
  char marchid_ch[8];
  for (int i = 0; i < 8; i++) {
    marchid_ch[i] = (marchid >> (i * 8)) & 0xFF;
  }
  char mimpid_ch[8];
  for (int i = 0; i < 8; i++) {
    mimpid_ch[i] = (mimpid >> (i * 8)) & 0xFF;
  }

  printf("\033[34m\n************ ID_INFO ************\n\033[0m");
  printf("\033[34mmvendorid_ch: \t%8s\n\033[0m", mvendorid_ch);
  printf("\033[34mmarchid_ch: \t%8s\n\033[0m", marchid_ch);
  printf("\033[34mmimpid_ch: \t%8s\n\033[0m", mimpid_ch);
  printf("\033[34m*********************************\n\n\033[0m");
}

void ARCH_Info(){
  printf("SYS: %x IDL: %x IDH: %x\n", ARCHINFO_REG_SYS, ARCHINFO_REG_IDL, ARCHINFO_REG_IDH);
}
void ARCH_SetSys(uint32_t val){
  ARCHINFO_REG_SYS = val;
}
void ARCH_SetIdl(uint32_t val){
  ARCHINFO_REG_IDL = val;
}
void ARCH_SetIdh(uint32_t val){
  ARCHINFO_REG_IDH = val;
}

void RNG_SetCtrl(uint32_t val){
  RNG_REG_CTRL = val;
}
void RNG_SetSeed(uint32_t val){
  RNG_REG_SEED = val;
}
uint32_t RNG_GetVal(){
  return RNG_REG_VAL;
}

void CRC_SetCtrl(uint32_t val){
  CRC_REG_CTRL = val;
}
void CRC_SetInit(uint32_t val){
  CRC_REG_INIT = val;
}
void CRC_SetXorV(uint32_t val){
  CRC_REG_XORV = val;
}
void CRC_SetData(uint32_t val){
  CRC_REG_DATA = val;
}
uint32_t CRC_GetVal(){
  return CRC_REG_DATA;
}

void WDG_SetCtrl(uint32_t val){
  WDG_REG_CTRL = val;
}

void WDG_SetPrescale(uint32_t val){
  WDG_REG_PSCR = val;
}

void WDG_SetCmp(uint32_t val){
  WDG_REG_CMP = val;
}

void WDG_Feed(uint32_t val){
  WDG_REG_FEED = val;
}

void WDG_SetKey(uint32_t val){
  WDG_REG_KEY = val;
}

uint32_t WDG_GetStat(){
  return WDG_REG_STAT;
}

void RCU_SetCtrl(uint32_t val){
  RCU_REG_CTRL = val;
}
void RCU_SetRdiv(uint32_t val){
  RCU_REG_RDIV = val;
}
uint32_t RCU_GetStat(){
  return RCU_REG_STAT;
}

void RTC_SetCtrl(uint32_t val){
  RTC_REG_CTRL = val;
}
void RTC_SetPrescale(uint32_t val){
  RTC_REG_PSCR = val - 1;
}
void RTC_SetCnt(uint32_t val){
  RTC_REG_CNT = val;
}
uint32_t RTC_GetCnt(){
  return RTC_REG_CNT;
}
void RTC_SetAlrm(uint32_t val){
  RTC_REG_ALRM = val;
}
uint32_t RTC_GetIsta(){
  return RTC_REG_ISTA;
}

void GPIO_SetDir(uint32_t gpio, uint32_t dir){
  if(gpio == GPIO_0){
      GPIO_0_REG_PADDIR = dir;
  }else if(gpio == GPIO_1){
      GPIO_1_REG_PADDIR = dir;
  }
}

void GPIO_SetDir_Num(uint32_t gpio, uint32_t pin, uint32_t dir){
  if(gpio == GPIO_0){
    if(dir == GPIO_OUT){
      GPIO_0_REG_PADDIR = GPIO_0_REG_PADDIR | (1 << pin);
    }else{
      GPIO_0_REG_PADDIR = GPIO_0_REG_PADDIR & ~(1 << pin);
    }
  }else if(gpio == GPIO_1){
    if(dir == GPIO_OUT){
      GPIO_1_REG_PADDIR = GPIO_1_REG_PADDIR | (1 << pin);
    }else{
      GPIO_1_REG_PADDIR = GPIO_1_REG_PADDIR & ~(1 << pin);
    }
  }
}

void GPIO_SetVal(uint32_t gpio, uint32_t val){
  if(gpio == GPIO_0){
      GPIO_0_REG_PADOUT = val;
  }else if(gpio == GPIO_1){
      GPIO_1_REG_PADOUT = val;
  }
}

void GPIO_SetVal_Num(uint32_t gpio, uint32_t pin, uint32_t val){
  if(gpio == GPIO_0){
    if(val == 1){
      GPIO_0_REG_PADOUT = GPIO_0_REG_PADOUT | (1 << pin);
    }else{
      GPIO_0_REG_PADOUT = GPIO_0_REG_PADOUT & ~(1 << pin);
    }
  }else if(gpio == GPIO_1){
    if(val == 1){
      GPIO_1_REG_PADOUT = GPIO_1_REG_PADOUT | (1 << pin);
    }else{
      GPIO_1_REG_PADOUT = GPIO_1_REG_PADOUT & ~(1 << pin);
    }
  }
}

void GPIO_SetFCFG(uint32_t gpio, uint32_t val){
  if(gpio == GPIO_0){
      GPIO_0_REG_IOFCFG = val;
  }else if(gpio == GPIO_1){
      GPIO_1_REG_IOFCFG = val;
  }
}

void GPIO_SetFCFG_Num(uint32_t gpio, uint32_t pin, uint32_t val){
  if(gpio == GPIO_0){
    if(val == 1){
      GPIO_0_REG_IOFCFG = GPIO_0_REG_IOFCFG | (1 << pin);
    }else{
      GPIO_0_REG_IOFCFG = GPIO_0_REG_IOFCFG & ~(1 << pin);
    }
  }else if(gpio == GPIO_1){
    if(val == 1){
      GPIO_1_REG_IOFCFG = GPIO_1_REG_IOFCFG | (1 << pin);
    }else{
      GPIO_1_REG_IOFCFG = GPIO_1_REG_IOFCFG & ~(1 << pin);
    }
  }
}

void GPIO_SetMUX(uint32_t gpio, uint32_t val){
  if(gpio == GPIO_0){
      GPIO_0_REG_PINMUX = val;
  }else if(gpio == GPIO_1){
      GPIO_1_REG_PINMUX = val;
  }
}

void GPIO_SetMUX_Num(uint32_t gpio, uint32_t pin, uint32_t val){
  if(gpio == GPIO_0){
    if(val == 1){
      GPIO_0_REG_PINMUX = GPIO_0_REG_PINMUX | (1 << pin);
    }else{
      GPIO_0_REG_PINMUX = GPIO_0_REG_PINMUX & ~(1 << pin);
    }
  }else if(gpio == GPIO_1){
    if(val == 1){
      GPIO_1_REG_PINMUX = GPIO_1_REG_PINMUX | (1 << pin);
    }else{
      GPIO_1_REG_PINMUX = GPIO_1_REG_PINMUX & ~(1 << pin);
    }
  }
}

uint32_t GPIO_GetVal(uint32_t gpio){
  if(gpio == GPIO_0){
      return GPIO_0_REG_PADIN;
  }else if(gpio == GPIO_1){
      return GPIO_1_REG_PADIN;
  }
  return 0;
}

uint32_t GPIO_GetVal_Num(uint32_t gpio, uint32_t pin){
  if(gpio == GPIO_0){
      return (GPIO_0_REG_PADIN >> pin) & 0x1;
  }else if(gpio == GPIO_1){
      return (GPIO_1_REG_PADIN >> pin) & 0x1;
  }
  return 0;
}

void timer_init(uint32_t div, uint32_t cmp) {
  printf("TIMER INIT:\n");
  TIMER_0_REG_CTRL = (uint32_t)0x0; // disable timer
  while (TIMER_0_REG_STAT == 1)
    ; // clear irq
  TIMER_0_REG_PSCR = div - 1;
  TIMER_0_REG_CMP = cmp - 1;

  printf("CTRL: %d PSCR: %d CMP: %d\n", TIMER_0_REG_CTRL, TIMER_0_REG_PSCR,
         TIMER_0_REG_CMP);
  printf("TIMER INIT DONE\n");
}

void delay_ms(uint32_t val) {
  TIMER_0_REG_CTRL = (uint32_t)0xD;
  for (int i = 0; i < val; ++i) {
    while (TIMER_0_REG_STAT == 0)
      ;
  }
  TIMER_0_REG_CTRL = (uint32_t)0xD;
}


void PWM_Init(uint32_t pwm, uint32_t prescale, uint32_t cmp){
  if(pwm == PWM_0){
    PWM_0_REG_CTRL = (uint32_t)0;
    PWM_0_REG_PSCR = prescale - 1; // 100M / prescale
    PWM_0_REG_CMP = cmp - 1;       // freq = 100M / prescale / cmp
    printf("PWM_0_REG_CTRL: %d PWM_0_REG_PSCR: %d PWM_0_REG_CMP: %d\n", PWM_0_REG_CTRL, PWM_0_REG_PSCR, PWM_0_REG_CMP);
  }else if(pwm == PWM_1){
    PWM_1_REG_CTRL = (uint32_t)0;
    PWM_1_REG_PSCR = prescale - 1; // 100M / prescale
    PWM_1_REG_CMP = cmp - 1;       // freq = 100M / prescale / cmp
    printf("PWM_1_REG_CTRL: %d PWM_1_REG_PSCR: %d PWM_1_REG_CMP: %d\n", PWM_1_REG_CTRL, PWM_1_REG_PSCR, PWM_1_REG_CMP);
  }else if(pwm == PWM_2){
    PWM_2_REG_CTRL = (uint32_t)0;
    PWM_2_REG_PSCR = prescale - 1; // 100M / prescale
    PWM_2_REG_CMP = cmp - 1;       // freq = 100M / prescale / cmp
    printf("PWM_2_REG_CTRL: %d PWM_2_REG_PSCR: %d PWM_2_REG_CMP: %d\n", PWM_2_REG_CTRL, PWM_2_REG_PSCR, PWM_2_REG_CMP);
  }
}
void PWM_SetCtr(uint32_t pwm, uint32_t val){
  if(pwm == PWM_0){
    PWM_0_REG_CTRL = val;
  }else if(pwm == PWM_1){
    PWM_1_REG_CTRL = val;
  }else if(pwm == PWM_2){
    PWM_2_REG_CTRL = val;
  }
}
void PWM_SetPrescale(uint32_t pwm, uint32_t val){
  if(pwm == PWM_0){
    PWM_0_REG_PSCR = val - 1;
  }else if(pwm == PWM_1){
    PWM_1_REG_PSCR = val - 1;
  }else if(pwm == PWM_2){
    PWM_2_REG_PSCR = val - 1;
  }
}
void PWM_SetCmp(uint32_t pwm, uint32_t val){
  if(pwm == PWM_0){
    PWM_0_REG_CMP = val - 1;
  }else if(pwm == PWM_1){
    PWM_1_REG_CMP = val - 1;
  }else if(pwm == PWM_2){
    PWM_2_REG_CMP = val - 1;
  }
}
void PWM_SetDuty(uint32_t pwm, uint32_t channel, uint32_t duty){
  if(pwm == PWM_0){
    if(channel == PWM_CHANAL_0){
      PWM_0_REG_CR0 = duty;
    }else if(channel == PWM_CHANAL_1){
      PWM_0_REG_CR1 = duty;
    }else if(channel == PWM_CHANAL_2){
      PWM_0_REG_CR2 = duty;
    }else if(channel == PWM_CHANAL_3){
      PWM_0_REG_CR3 = duty;
    }
  }else if(pwm == PWM_1){
    if(channel == PWM_CHANAL_0){
      PWM_1_REG_CR0 = duty;
    }else if(channel == PWM_CHANAL_1){
      PWM_1_REG_CR1 = duty;
    }else if(channel == PWM_CHANAL_2){
      PWM_1_REG_CR2 = duty;
    }else if(channel == PWM_CHANAL_3){
      PWM_1_REG_CR3 = duty;
    }
  }else if(pwm == PWM_2){
    if(channel == PWM_CHANAL_0){
      PWM_2_REG_CR0 = duty;
    }else if(channel == PWM_CHANAL_1){
      PWM_2_REG_CR1 = duty;
    }else if(channel == PWM_CHANAL_2){
      PWM_2_REG_CR2 = duty;
    }else if(channel == PWM_CHANAL_3){
      PWM_2_REG_CR3 = duty;
    }
  }
}

// spi1

void SPI1_Init(){
  printf("SPI INIT:\n");
  SPI1_REG_STATUS = (uint32_t)0b10000;
  SPI1_REG_STATUS = (uint32_t)0b00000;
  SPI1_REG_INTCFG = (uint32_t)0b00000;
  SPI1_REG_DUM = (uint32_t)0;
  SPI1_REG_CLKDIV = (uint32_t)4; // sck = apb_clk/2(div+1) 100MHz/2 = 50MHz
  printf("SPI1_STATUS: %08x\n", SPI1_REG_STATUS);
  printf("SPI1_CLKDIV: %08x\n", SPI1_REG_CLKDIV);
  printf("SPI1_INTCFG: %08x\n", SPI1_REG_INTCFG);
  printf("SPI1_DUM: %08x\n", SPI1_REG_DUM); 
  printf("SPI INIT DONE\n"); 
}

void spi1_wr_dat(uint8_t dat) {
  uint32_t wdat = ((uint32_t)dat) << 24;
  SPI1_REG_LEN = 0x80000;
  SPI1_REG_TXFIFO = wdat;
  SPI1_REG_STATUS = 258;
  while ((SPI1_REG_STATUS & 0xFFFF) != 1)
    ;
}

// spi1_tft
void spi_tft_init() {
  printf("GPIO INIT:\n");
  GPIO_1_REG_IOFCFG |= (uint32_t)(1 << 0);
  GPIO_1_REG_IOFCFG |= (uint32_t)(1 << 1);
  GPIO_1_REG_IOFCFG |= (uint32_t)(1 << 2);
  GPIO_1_REG_IOFCFG |= (uint32_t)(1 << 3);
  GPIO_1_REG_IOFCFG |= (uint32_t)(1 << 4);
  GPIO_1_REG_IOFCFG |= (uint32_t)(1 << 5);
  GPIO_1_REG_PINMUX = 0; // FUNC0
  GPIO_1_REG_PADDIR = (uint32_t)(1 << 6);

  printf("GPIO_1_PADDIR: %08x\n", GPIO_1_REG_PADDIR);
  printf("GPIO_1_IOFCFG: %08x\n", GPIO_1_REG_IOFCFG);
  printf("GPIO_1_PINMUX: %08x\n", GPIO_1_REG_PINMUX);
  printf("GPIO INIT DONE\n");
  SPI1_Init();
  printf("tft init begin\n");

  delay_ms(500);

  printf("exit sleep\n");
  lcd_wr_cmd(0x11); // 睡眠退出
  delay_ms(500);

  printf("exit sleep\n");
  // ST7735R 帧速率
  lcd_wr_cmd(0xB1);
  lcd_wr_data8(0x01);
  lcd_wr_data8(0x2C);
  lcd_wr_data8(0x2D);
  lcd_wr_cmd(0xB2);
  lcd_wr_data8(0x01);
  lcd_wr_data8(0x2C);
  lcd_wr_data8(0x2D);
  lcd_wr_cmd(0xB3);
  lcd_wr_data8(0x01);
  lcd_wr_data8(0x2C);
  lcd_wr_data8(0x2D);
  lcd_wr_data8(0x01);
  lcd_wr_data8(0x2C);
  lcd_wr_data8(0x2D);
  lcd_wr_cmd(0xB4); // 列反转
  lcd_wr_data8(0x07);
  printf("after frame rate init\n");

  // ST7735R Power Sequence
  lcd_wr_cmd(0xC0);
  lcd_wr_data8(0xA2);
  lcd_wr_data8(0x02);
  lcd_wr_data8(0x84);
  lcd_wr_cmd(0xC1);
  lcd_wr_data8(0xC5);
  lcd_wr_cmd(0xC2);
  lcd_wr_data8(0x0A);
  lcd_wr_data8(0x00);
  lcd_wr_cmd(0xC3);
  lcd_wr_data8(0x8A);
  lcd_wr_data8(0x2A);
  lcd_wr_cmd(0xC4);
  lcd_wr_data8(0x8A);
  lcd_wr_data8(0xEE);
  lcd_wr_cmd(0xC5); // VCOM
  lcd_wr_data8(0x0E);
  lcd_wr_cmd(0x36); // MX,MY,RGB mode
  printf("after power sequence init\n");

  switch (USE_HORIZONTAL) //显示的方向(竖屏:0,横屏:1,竖屏旋转180度:2,横屏旋转180度:3)
  {
  case 0:
    lcd_wr_data8(0xC8);
    break; // 竖屏
  case 1:
    lcd_wr_data8(0xA8);
    break; // 横屏
  case 2:
    lcd_wr_data8(0x08);
    break; // 竖屏翻转180度
  default:
    lcd_wr_data8(0x68);
    break; // 横屏翻转180度
  }

  // ST7735R Gamma Sequence
  lcd_wr_cmd(0xE0);
  lcd_wr_data8(0x0F);
  lcd_wr_data8(0x1A);
  lcd_wr_data8(0x0F);
  lcd_wr_data8(0x18);
  lcd_wr_data8(0x2F);
  lcd_wr_data8(0x28);
  lcd_wr_data8(0x20);
  lcd_wr_data8(0x22);
  lcd_wr_data8(0x1F);
  lcd_wr_data8(0x1B);
  lcd_wr_data8(0x23);
  lcd_wr_data8(0x37);
  lcd_wr_data8(0x00);
  lcd_wr_data8(0x07);
  lcd_wr_data8(0x02);
  lcd_wr_data8(0x10);

  lcd_wr_cmd(0xE1);
  lcd_wr_data8(0x0F);
  lcd_wr_data8(0x1B);
  lcd_wr_data8(0x0F);
  lcd_wr_data8(0x17);
  lcd_wr_data8(0x33);
  lcd_wr_data8(0x2C);
  lcd_wr_data8(0x29);
  lcd_wr_data8(0x2E);
  lcd_wr_data8(0x30);
  lcd_wr_data8(0x30);
  lcd_wr_data8(0x39);
  lcd_wr_data8(0x3F);
  lcd_wr_data8(0x00);
  lcd_wr_data8(0x07);
  lcd_wr_data8(0x03);
  lcd_wr_data8(0x10);
  printf("after gamma sequence init\n");

  lcd_wr_cmd(0xF0); // 启动测试命令
  lcd_wr_data8(0x01);
  lcd_wr_cmd(0xF6); // 禁用ram省电模式
  lcd_wr_data8(0x00);

  lcd_wr_cmd(0x3A); // 65k mode
  lcd_wr_data8(0x05);
  lcd_wr_cmd(0x29); // 开启显示
  lcd_fill(0, 0, 128, 128, 0xFFFF);
  printf("TFT init done\n");
}

void lcd_fill_bmp(const uint8_t *bmp, uint16_t x, uint16_t y, uint16_t w,
                  uint16_t h) {
  lcd_addr_set(x, y, x + w - 1, y + h - 1);
  for (uint16_t i = 0; i < h * w * 2; i++) {
    lcd_wr_data8(bmp[i]);
  }
}

void lcd_refresh(uint16_t *gdm, uint16_t x, uint16_t y, uint16_t w,
                 uint16_t h) {
  lcd_addr_set(x, y, x + w - 1, y + h - 1);
  for (uint16_t i = 0; i < h; i++) {
    for (uint16_t j = 0; j < w; j++) {
      lcd_wr_data16(gdm[i * 128 + j]);
    }
  }
}

void lcd_wr_data32x32(uint32_t dat1, uint32_t dat2, uint32_t dat3,
                      uint32_t dat4, uint32_t dat5, uint32_t dat6,
                      uint32_t dat7, uint32_t dat8, uint32_t dat9,
                      uint32_t dat10, uint32_t dat11, uint32_t dat12,
                      uint32_t dat13, uint32_t dat14, uint32_t dat15,
                      uint32_t dat16, uint32_t dat17, uint32_t dat18,
                      uint32_t dat19, uint32_t dat20, uint32_t dat21,
                      uint32_t dat22, uint32_t dat23, uint32_t dat24,
                      uint32_t dat25, uint32_t dat26, uint32_t dat27,
                      uint32_t dat28, uint32_t dat29, uint32_t dat30,
                      uint32_t dat31, uint32_t dat32) {
  lcd_dc_set;

  SPI1_REG_LEN = 0x4000000; // NOTE: 32x32bits
  SPI1_REG_TXFIFO = dat1;
  SPI1_REG_TXFIFO = dat2;
  SPI1_REG_TXFIFO = dat3;
  SPI1_REG_TXFIFO = dat4;
  SPI1_REG_TXFIFO = dat5;
  SPI1_REG_TXFIFO = dat6;
  SPI1_REG_TXFIFO = dat7;
  SPI1_REG_TXFIFO = dat8;
  SPI1_REG_TXFIFO = dat9;
  SPI1_REG_TXFIFO = dat10;
  SPI1_REG_TXFIFO = dat11;
  SPI1_REG_TXFIFO = dat12;
  SPI1_REG_TXFIFO = dat13;
  SPI1_REG_TXFIFO = dat14;
  SPI1_REG_TXFIFO = dat15;
  SPI1_REG_TXFIFO = dat16;
  SPI1_REG_TXFIFO = dat17;
  SPI1_REG_TXFIFO = dat18;
  SPI1_REG_TXFIFO = dat19;
  SPI1_REG_TXFIFO = dat20;
  SPI1_REG_TXFIFO = dat21;
  SPI1_REG_TXFIFO = dat22;
  SPI1_REG_TXFIFO = dat23;
  SPI1_REG_TXFIFO = dat24;
  SPI1_REG_TXFIFO = dat25;
  SPI1_REG_TXFIFO = dat26;
  SPI1_REG_TXFIFO = dat27;
  SPI1_REG_TXFIFO = dat28;
  SPI1_REG_TXFIFO = dat29;
  SPI1_REG_TXFIFO = dat30;
  SPI1_REG_TXFIFO = dat31;
  SPI1_REG_TXFIFO = dat32;
  SPI1_REG_STATUS = 258;
  while ((SPI1_REG_STATUS & 0xFFFF) != 1)
    ;
}

void lcd_fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend,
              uint32_t color) {
  lcd_addr_set(xsta, ysta, xend - 1, yend - 1);
  for (uint16_t i = ysta; i < yend; ++i) {
    for (uint16_t j = xsta; j < xend; j += 64) {
      lcd_wr_data32x32(color, color, color, color, color, color, color, color,
                       color, color, color, color, color, color, color, color,
                       color, color, color, color, color, color, color, color,
                       color, color, color, color, color, color, color, color);
    }
  }
}

#define LCD_X_OFFSET 2
#define LCD_Y_OFFSET 3
void lcd_addr_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  lcd_wr_cmd(0x2A);
  lcd_wr_data16(x1 + LCD_X_OFFSET);
  lcd_wr_data16(x2 + LCD_X_OFFSET);
  lcd_wr_cmd(0x2B);
  lcd_wr_data16(y1+ LCD_Y_OFFSET);
  lcd_wr_data16(y2 + LCD_Y_OFFSET);
  lcd_wr_cmd(0x2C);
}

void lcd_wr_cmd(uint8_t cmd) {
  lcd_dc_clr;
  spi1_wr_dat(cmd);
}

void lcd_wr_data8(uint8_t dat) {
  lcd_dc_set;
  spi1_wr_dat(dat);
}

void lcd_wr_data16(uint16_t dat){
  lcd_dc_set;

  uint32_t wdat = ((uint32_t)dat) << 16;
  SPI1_REG_LEN = 0x100000;
  SPI1_REG_TXFIFO = wdat;
  SPI1_REG_STATUS = 258;
  while ((SPI1_REG_STATUS & 0xFFFF) != 1)
    ;
}

//ps2

void PS2_SetCtrl(uint32_t val){
  PS2_REG_CTRL = val;
}
uint32_t PS2_GetData(){
  return PS2_REG_DATA;
}

// i2c
void i2c_config() {
  GPIO_0_REG_IOFCFG = GPIO_0_REG_IOFCFG | (1 << 29);
  GPIO_0_REG_IOFCFG = GPIO_0_REG_IOFCFG | (1 << 30);
  GPIO_0_REG_PINMUX = GPIO_0_REG_PINMUX & ~(1 << 29) & ~(1 << 30);
  printf("GPIO_0_PADDIR: %08x\n", GPIO_0_REG_IOFCFG);
  printf("GPIO_0_PINMUX: %08x\n", GPIO_0_REG_PINMUX);
  I2C_REG_CTRL = (uint8_t)0;
  I2C_REG_PSCRH = (uint8_t)0;
  I2C_REG_PSCRL = (uint8_t)(100 - 1); // 100MHz / (5 * 100KHz) - 1

  printf("CTRL: %08x PSCR: %d\n", I2C_REG_CTRL, I2C_REG_PSCRL);
  I2C_REG_CTRL = (uint8_t)0x80; // core en
  printf("CTRL: %08x PSCR: %d\n", I2C_REG_CTRL, I2C_REG_PSCRL);
  printf("status: %08x\n", I2C_REG_SR);
}

uint32_t i2c_get_ack() {
  while ((I2C_REG_SR & I2C_STATUS_TIP) == 0)
    ;
  while ((I2C_REG_SR & I2C_STATUS_TIP) != 0)
    ;
  return !(I2C_REG_SR & I2C_STATUS_RXACK); // invert since signal is active low
}

uint32_t i2c_busy() {
  return ((I2C_REG_SR & I2C_STATUS_BUSY) == I2C_STATUS_BUSY);
}

void i2c_wr_start(uint32_t slv_addr) {
  I2C_REG_TXR = slv_addr;
  I2C_REG_CMD = I2C_TEST_START_WRITE;
  if (!i2c_get_ack())
    putstr("[wr start]no ack recv\n");
}

void i2c_rd_start(uint32_t slv_addr) {
  do {
    I2C_REG_TXR = slv_addr;
    I2C_REG_CMD = I2C_TEST_START_WRITE;
  } while (!i2c_get_ack());
}

void i2c_write(uint8_t val) {
  I2C_REG_TXR = val;
  I2C_REG_CMD = I2C_TEST_WRITE;
  if (!i2c_get_ack())
    putstr("[i2c write]no ack recv\n");
}

uint32_t i2c_read(uint32_t cmd) {
  I2C_REG_CMD = cmd;
  if (!i2c_get_ack())
    putstr("[i2c read]no ack recv\n");
  return I2C_REG_RXR;
}

void i2c_stop() {
  I2C_REG_CMD = I2C_TEST_STOP;
  while (i2c_busy())
    ;
}

void i2c_wr_nbyte(uint8_t slv_addr, uint16_t reg_addr, uint8_t type,
                  uint8_t num, uint8_t *data) {
  i2c_rd_start(slv_addr);
  if (type == I2C_DEV_ADDR_16BIT) {
    i2c_write((uint8_t)((reg_addr >> 8) & 0xFF));
    i2c_write((uint8_t)(reg_addr & 0xFF));
  } else {
    i2c_write((uint8_t)(reg_addr & 0xFF));
  }
  for (int i = 0; i < num; ++i) {
    i2c_write(*data);
    ++data;
  }
  i2c_stop();
}

void i2c_rd_nbyte(uint8_t slv_addr, uint16_t reg_addr, uint8_t type,
                  uint8_t num, uint8_t *data) {
  i2c_rd_start(slv_addr);
  if (type == I2C_DEV_ADDR_16BIT) {
    i2c_write((uint8_t)((reg_addr >> 8) & 0xFF));
    i2c_write((uint8_t)(reg_addr & 0xFF));
  } else {
    i2c_write((uint8_t)(reg_addr & 0xFF));
  }
  i2c_stop();

  i2c_wr_start(slv_addr + 1);
  for (int i = 0; i < num; ++i) {
    if (i == num - 1)
      data[i] = i2c_read(I2C_TEST_STOP_READ);
    else
      data[i] = i2c_read(I2C_TEST_READ);
  }
}
