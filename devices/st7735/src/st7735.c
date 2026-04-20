#include "st7735.h"
#include "stdio.h"
#include "hal_gpio.h"
#include "hal_qspi.h"

// Assuming hal_timer_delay_ms or similar is available, if not, a local delay or a known hal delay.
// But earlier we saw L3_1 using a custom delay in templates. Let's use a weak or simple loop if hal doesn't provide.
// Wait, we can declare a weak delay_ms, or just include hal_timer if it provides it.
// Let's rely on user application to provide delay_ms as it was done previously.
extern void delay_ms(uint32_t val);

void st7735_wr_data8(st7735_device_t* dev, uint8_t data){
  gpio_hal_set_level(dev->dc_gpio_port, dev->dc_gpio_pin, GPIO_LEVEL_HIGH);
  hal_qspi_write_8_cs(dev->qspi_port, data, dev->qspi_cs);
}

void st7735_wr_data16(st7735_device_t* dev, uint16_t data){
  gpio_hal_set_level(dev->dc_gpio_port, dev->dc_gpio_pin, GPIO_LEVEL_HIGH);
  hal_qspi_write_16_cs(dev->qspi_port, data, dev->qspi_cs);
}

void st7735_wr_data32(st7735_device_t* dev, uint32_t data){
  gpio_hal_set_level(dev->dc_gpio_port, dev->dc_gpio_pin, GPIO_LEVEL_HIGH);
  hal_qspi_write_32_cs(dev->qspi_port, data, dev->qspi_cs);
}

void st7735_wr_data32x2(st7735_device_t* dev, uint32_t data1, uint32_t data2){
  gpio_hal_set_level(dev->dc_gpio_port, dev->dc_gpio_pin, GPIO_LEVEL_HIGH);
  hal_qspi_write_32x2_cs(dev->qspi_port, data1, data2, dev->qspi_cs);
}

void st7735_wr_data32x8(st7735_device_t* dev, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8){
  gpio_hal_set_level(dev->dc_gpio_port, dev->dc_gpio_pin, GPIO_LEVEL_HIGH);
  hal_qspi_write_32x8_cs(dev->qspi_port, data1, data2, data3, data4, data5, data6, data7, data8, dev->qspi_cs);
}

void st7735_wr_data32x16(st7735_device_t* dev, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8,
     uint32_t data9, uint32_t data10, uint32_t data11, uint32_t data12, uint32_t data13, uint32_t data14, uint32_t data15, uint32_t data16){
  gpio_hal_set_level(dev->dc_gpio_port, dev->dc_gpio_pin, GPIO_LEVEL_HIGH);
  hal_qspi_write_32x16_cs(dev->qspi_port, data1, data2, data3, data4, data5, data6, data7, data8, data9, data10, data11, data12, data13, data14, data15, data16, dev->qspi_cs);
}

void st7735_wr_data32x32(st7735_device_t* dev, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8,
     uint32_t data9, uint32_t data10, uint32_t data11, uint32_t data12, uint32_t data13, uint32_t data14, uint32_t data15, uint32_t data16,
     uint32_t data17, uint32_t data18, uint32_t data19, uint32_t data20, uint32_t data21, uint32_t data22, uint32_t data23, uint32_t data24,
     uint32_t data25, uint32_t data26, uint32_t data27, uint32_t data28, uint32_t data29, uint32_t data30, uint32_t data31, uint32_t data32){
  gpio_hal_set_level(dev->dc_gpio_port, dev->dc_gpio_pin, GPIO_LEVEL_HIGH);   
  hal_qspi_write_32x32_cs(dev->qspi_port, data1, data2, data3, data4, data5, data6, data7, data8, data9, data10, data11, data12, data13, data14, data15, data16,
    data17, data18, data19, data20, data21, data22, data23, data24, data25, data26, data27, data28, data29, data30, data31, data32, dev->qspi_cs);
}

void st7735_wr_cmd(st7735_device_t* dev, uint8_t cmd){
    gpio_hal_set_level(dev->dc_gpio_port, dev->dc_gpio_pin, GPIO_LEVEL_LOW);
    hal_qspi_write_8_cs(dev->qspi_port, cmd, dev->qspi_cs);
}

void st7735_init(st7735_device_t* dev){
    // dc pin
    gpio_hal_output_enable(dev->dc_gpio_port, dev->dc_gpio_pin);
    gpio_hal_set_level(dev->dc_gpio_port, dev->dc_gpio_pin, GPIO_LEVEL_LOW);

    delay_ms(120);
    st7735_wr_cmd(dev, 0x11); // 睡眠退出
    delay_ms(120);
    st7735_wr_cmd(dev, 0xB1);
    st7735_wr_data8(dev, 0x01);
    st7735_wr_data8(dev, 0x2C);
    st7735_wr_data8(dev, 0x2D);
    st7735_wr_cmd(dev, 0xB2);
    st7735_wr_data8(dev, 0x01);
    st7735_wr_data8(dev, 0x2C);
    st7735_wr_data8(dev, 0x2D);
    st7735_wr_cmd(dev, 0xB3);
    st7735_wr_data8(dev, 0x01);
    st7735_wr_data8(dev, 0x2C);
    st7735_wr_data8(dev, 0x2D);
    st7735_wr_data8(dev, 0x01);
    st7735_wr_data8(dev, 0x2C);
    st7735_wr_data8(dev, 0x2D);
    st7735_wr_cmd(dev, 0xB4); // 列反转
    st7735_wr_data8(dev, 0x07);
    printf("after frame rate init\n");

    // ST7735R Power Sequence
    st7735_wr_cmd(dev, 0xC0);
    st7735_wr_data8(dev, 0xA2);
    st7735_wr_data8(dev, 0x02);
    st7735_wr_data8(dev, 0x84);
    st7735_wr_cmd(dev, 0xC1);
    st7735_wr_data8(dev, 0xC5);
    st7735_wr_cmd(dev, 0xC2);
    st7735_wr_data8(dev, 0x0A);
    st7735_wr_data8(dev, 0x00);
    st7735_wr_cmd(dev, 0xC3);
    st7735_wr_data8(dev, 0x8A);
    st7735_wr_data8(dev, 0x2A);
    st7735_wr_cmd(dev, 0xC4);
    st7735_wr_data8(dev, 0x8A);
    st7735_wr_data8(dev, 0xEE);
    st7735_wr_cmd(dev, 0xC5); // VCOM
    st7735_wr_data8(dev, 0x0E);
    st7735_wr_cmd(dev, 0x36); // MX,MY,RGB mode
    printf("after power sequence init\n");

    switch (dev->rotation) // 显示的方向(竖屏:0,横屏:1,竖屏旋转180度:2,横屏旋转180度:3)
    {
    case 0:
      st7735_wr_data8(dev, 0xC8);
      break; // 竖屏
    case 1:
      st7735_wr_data8(dev, 0xA8);
      break; // 横屏
    case 2:
      st7735_wr_data8(dev, 0x08);
      break; // 竖屏翻转180度
    default:
      st7735_wr_data8(dev, 0x68);
      break; // 横屏翻转180度
    }

    // ST7735R Gamma Sequence
    st7735_wr_cmd(dev, 0xE0);
    st7735_wr_data8(dev, 0x0F);
    st7735_wr_data8(dev, 0x1A);
    st7735_wr_data8(dev, 0x0F);
    st7735_wr_data8(dev, 0x18);
    st7735_wr_data8(dev, 0x2F);
    st7735_wr_data8(dev, 0x28);
    st7735_wr_data8(dev, 0x20);
    st7735_wr_data8(dev, 0x22);
    st7735_wr_data8(dev, 0x1F);
    st7735_wr_data8(dev, 0x1B);
    st7735_wr_data8(dev, 0x23);
    st7735_wr_data8(dev, 0x37);
    st7735_wr_data8(dev, 0x00);
    st7735_wr_data8(dev, 0x07);
    st7735_wr_data8(dev, 0x02);
    st7735_wr_data8(dev, 0x10);

    st7735_wr_cmd(dev, 0xE1);
    st7735_wr_data8(dev, 0x0F);
    st7735_wr_data8(dev, 0x1B);
    st7735_wr_data8(dev, 0x0F);
    st7735_wr_data8(dev, 0x17);
    st7735_wr_data8(dev, 0x33);
    st7735_wr_data8(dev, 0x2C);
    st7735_wr_data8(dev, 0x29);
    st7735_wr_data8(dev, 0x2E);
    st7735_wr_data8(dev, 0x30);
    st7735_wr_data8(dev, 0x30);
    st7735_wr_data8(dev, 0x39);
    st7735_wr_data8(dev, 0x3F);
    st7735_wr_data8(dev, 0x00);
    st7735_wr_data8(dev, 0x07);
    st7735_wr_data8(dev, 0x03);
    st7735_wr_data8(dev, 0x10);
    printf("after gamma sequence init\n");

    st7735_wr_cmd(dev, 0xF0); // 启动测试命令
    st7735_wr_data8(dev, 0x01);
    st7735_wr_cmd(dev, 0xF6); // 禁用ram省电模式
    st7735_wr_data8(dev, 0x00);

    st7735_wr_cmd(dev, 0x3A); // 65k mode
    st7735_wr_data8(dev, 0x05);
    st7735_wr_cmd(dev, 0x29); // 开启显示

    st7735_fill(dev, 0, 0, 128, 128, 0xFFFFFFFF);
    printf("TFT init done\n");
}

void st7735_addr_set(st7735_device_t* dev, uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend){
    st7735_wr_cmd(dev, 0x2A); // 列地址设置
    st7735_wr_data16(dev, xsta + dev->horizontal_offset);
    st7735_wr_data16(dev, xend + dev->horizontal_offset);
    st7735_wr_cmd(dev, 0x2B); // 行地址设置
    st7735_wr_data16(dev, ysta + dev->vertical_offset);
    st7735_wr_data16(dev, yend + dev->vertical_offset);
    st7735_wr_cmd(dev, 0x2C); // 内存写入
}

void st7735_fill(st7735_device_t* dev, uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint32_t color){
    st7735_addr_set(dev, xsta, ysta, xend - 1, yend - 1);
    for (uint16_t i = ysta; i < yend; i++){
        for (uint16_t j = xsta; j < xend; j += 64){
            st7735_wr_data32x32(dev, color, color, color, color, color, color, color, color, color, color, color, color, color, color, color, color, 
              color, color, color, color, color, color, color, color, color, color, color, color, color, color, color, color);
        }
    }
}

void st7735_fill_img(st7735_device_t* dev, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint32_t *data) {
  st7735_addr_set(dev, x, y, x + w - 1, y + h - 1);
  for (uint16_t i = 0; i < 8192; i+=32) {
    st7735_wr_data32x32(dev, data[i],data[i+1],data[i+2],data[i+3],data[i+4],data[i+5],data[i+6],data[i+7],data[i+8],data[i+9],data[i+10],data[i+11],data[i+12],data[i+13],data[i+14],data[i+15],
        data[i+16],data[i+17],data[i+18],data[i+19],data[i+20],data[i+21],data[i+22],data[i+23],data[i+24],data[i+25],data[i+26],data[i+27],data[i+28],data[i+29],data[i+30],data[i+31]);
  }
}