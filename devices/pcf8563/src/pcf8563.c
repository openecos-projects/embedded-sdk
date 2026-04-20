#include "pcf8563.h"
#include "hal_i2c.h"
#include <stddef.h>

#define PCF8563_CTL_STATUS1   ((uint8_t)0x00)
#define PCF8563_CTL_STATUS2   ((uint8_t)0x01)
#define PCF8563_SECOND_REG    ((uint8_t)0x02)
#define PCF8563_MINUTE_REG    ((uint8_t)0x03)
#define PCF8563_HOUR_REG      ((uint8_t)0x04)
#define PCF8563_DAY_REG       ((uint8_t)0x05)
#define PCF8563_WEEKDAY_REG   ((uint8_t)0x06)
#define PCF8563_MONTH_REG     ((uint8_t)0x07)
#define PCF8563_YEAR_REG      ((uint8_t)0x08)

#define SECOND_MINUTE_REG_WIDTH ((uint8_t)0x7F)
#define HOUR_DAY_REG_WIDTH      ((uint8_t)0x3F)
#define WEEKDAY_REG_WIDTH       ((uint8_t)0x07)
#define MONTH_REG_WIDTH         ((uint8_t)0x1F)
#define YEAR_REG_WIDTH          ((uint8_t)0xFF)

static uint8_t pcf8563_bin2bcd(uint8_t val) {
  uint8_t bcdhigh = 0;
  while (val >= 10) {
    ++bcdhigh;
    val -= 10;
  }
  return ((uint8_t)(bcdhigh << 4) | val);
}

static uint8_t pcf8563_bcd2bin(uint8_t val, uint8_t reg_width) {
  uint8_t res = 0;
  res = (val & (reg_width & 0xF0)) >> 4;
  res = res * 10 + (val & (reg_width & 0x0F));
  return res;
}

void pcf8563_init(pcf8563_device_t* dev) {
    // A dummy init to ensure structure matches normal devices pattern.
    // If needed, we can clear STOP bit or configuration regs here.
}

void pcf8563_write_info(pcf8563_device_t* dev, const pcf8563_info_t *info) {
  if(!dev || !info) return;
  uint8_t wr_data[7] = {0};
  
  wr_data[0] = pcf8563_bin2bcd(info->time.second);
  wr_data[1] = pcf8563_bin2bcd(info->time.minute);
  wr_data[2] = pcf8563_bin2bcd(info->time.hour);
  wr_data[3] = pcf8563_bin2bcd(info->date.day);
  wr_data[4] = pcf8563_bin2bcd(info->date.weekday);
  wr_data[5] = pcf8563_bin2bcd(info->date.month);
  wr_data[6] = pcf8563_bin2bcd(info->date.year);
  
  hal_i2c_write_nbyte(dev->i2c_port, dev->i2c_addr, PCF8563_SECOND_REG, HAL_I2C_REG_8, wr_data, 7);
}

pcf8563_info_t pcf8563_read_info(pcf8563_device_t* dev) {
  pcf8563_info_t info = {0};
  if(!dev) return info;
  
  uint8_t rd_data[7] = {0};
  hal_i2c_read_nbyte(dev->i2c_port, dev->i2c_addr, PCF8563_SECOND_REG, HAL_I2C_REG_8, rd_data, 7);
  
  info.time.second = pcf8563_bcd2bin(rd_data[0], SECOND_MINUTE_REG_WIDTH);
  info.time.minute = pcf8563_bcd2bin(rd_data[1], SECOND_MINUTE_REG_WIDTH);
  info.time.hour = pcf8563_bcd2bin(rd_data[2], HOUR_DAY_REG_WIDTH);
  info.date.day = pcf8563_bcd2bin(rd_data[3], HOUR_DAY_REG_WIDTH);
  info.date.weekday = pcf8563_bcd2bin(rd_data[4], WEEKDAY_REG_WIDTH);
  info.date.month = pcf8563_bcd2bin(rd_data[5], MONTH_REG_WIDTH);
  info.date.year = pcf8563_bcd2bin(rd_data[6], YEAR_REG_WIDTH);
  
  return info;
}
