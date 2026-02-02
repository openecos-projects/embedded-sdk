#ifndef __ST7789_H__
#define __ST7789_H__

#include "st7789_type.h"

void st7789_init(st7789_device_t* dev);
void st7789_wr_cmd(st7789_device_t* dev, uint8_t cmd);
void st7789_wr_data8(st7789_device_t* dev, uint8_t data);
void st7789_wr_data16(st7789_device_t* dev, uint16_t data);
void st7789_wr_data32(st7789_device_t* dev, uint32_t data);
void st7789_wr_data32x2(st7789_device_t* dev, uint32_t data1, uint32_t data2);
void st7789_wr_data32x8(st7789_device_t* dev, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8);
void st7789_wr_data32x16(st7789_device_t* dev, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8,
     uint32_t data9, uint32_t data10, uint32_t data11, uint32_t data12, uint32_t data13, uint32_t data14, uint32_t data15, uint32_t data16);
void st7789_wr_data32x32(st7789_device_t* dev, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8,
     uint32_t data9, uint32_t data10, uint32_t data11, uint32_t data12, uint32_t data13, uint32_t data14, uint32_t data15, uint32_t data16,
     uint32_t data17, uint32_t data18, uint32_t data19, uint32_t data20, uint32_t data21, uint32_t data22, uint32_t data23, uint32_t data24,
     uint32_t data25, uint32_t data26, uint32_t data27, uint32_t data28, uint32_t data29, uint32_t data30, uint32_t data31, uint32_t data32);

void st7789_addr_set(st7789_device_t* dev, uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend);


void st7789_fill(st7789_device_t* dev, uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint32_t color);
void st7789_fill_img(st7789_device_t* dev, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint32_t *data);
#endif