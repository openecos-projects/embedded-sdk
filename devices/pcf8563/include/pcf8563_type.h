#ifndef PCF8563_TYPE_H
#define PCF8563_TYPE_H

#include <stdint.h>
#include "hal_i2c.h"

// I2C RTC Device Handle
typedef struct {
    hal_i2c_port_t i2c_port;
    uint8_t i2c_addr;
} pcf8563_device_t;

// PCF8563 Date and Time Info Structure
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
} pcf8563_info_t;

#endif
