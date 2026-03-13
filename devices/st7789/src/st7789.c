#include "st7789.h"
#include "stdio.h"
#include "gpio.h"
#include "qspi.h"
#include "timer.h"

void st7789_wr_data8(st7789_device_t* dev, uint8_t data){
  gpio_set_level(dev->dc_pin, 1);
  qspi_write_8(data);
}

void st7789_wr_data16(st7789_device_t* dev, uint16_t data){
  gpio_set_level(dev->dc_pin, 1);
  qspi_write_16(data);
}

void st7789_wr_data32(st7789_device_t* dev, uint32_t data){
  gpio_set_level(dev->dc_pin, 1);
  qspi_write_32(data);
}


void st7789_wr_data32x2(st7789_device_t* dev, uint32_t data1, uint32_t data2){
  gpio_set_level(dev->dc_pin, 1);
  qspi_write_32x2(data1, data2);
}

void st7789_wr_data32x8(st7789_device_t* dev, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8){
  gpio_set_level(dev->dc_pin, 1);
  qspi_write_32x8(data1, data2, data3, data4, data5, data6, data7, data8);
}

void st7789_wr_data32x16(st7789_device_t* dev, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8,
     uint32_t data9, uint32_t data10, uint32_t data11, uint32_t data12, uint32_t data13, uint32_t data14, uint32_t data15, uint32_t data16){
  gpio_set_level(dev->dc_pin, 1);
  qspi_write_32x16(data1, data2, data3, data4, data5, data6, data7, data8, data9, data10, data11, data12, data13, data14, data15, data16);
}

void st7789_wr_data32x32(st7789_device_t* dev, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4, uint32_t data5, uint32_t data6, uint32_t data7, uint32_t data8,
     uint32_t data9, uint32_t data10, uint32_t data11, uint32_t data12, uint32_t data13, uint32_t data14, uint32_t data15, uint32_t data16,
     uint32_t data17, uint32_t data18, uint32_t data19, uint32_t data20, uint32_t data21, uint32_t data22, uint32_t data23, uint32_t data24,
     uint32_t data25, uint32_t data26, uint32_t data27, uint32_t data28, uint32_t data29, uint32_t data30, uint32_t data31, uint32_t data32){
  gpio_set_level(dev->dc_pin, 1);   
  qspi_write_32x32(data1, data2, data3, data4, data5, data6, data7, data8, data9, data10, data11, data12, data13, data14, data15, data16,
    data17, data18, data19, data20, data21, data22, data23, data24, data25, data26, data27, data28, data29, data30, data31, data32);
}

void st7789_wr_cmd(st7789_device_t* dev, uint8_t cmd){
    gpio_set_level(dev->dc_pin, 0);
    qspi_write_8(cmd);
}

void st7789_init(st7789_device_t* dev){
    gpio_set_level(dev->rst_pin,GPIO_LEVEL_LOW);
    delay_ms(100);
    gpio_set_level(dev->rst_pin,GPIO_LEVEL_HIGH);
    delay_ms(100);

	//************* Start Initial Sequence **********//
	st7789_wr_cmd(dev, 0x11); //Sleep out 
	delay_ms(120);              //Delay 120ms 
	//************* Start Initial Sequence **********// 
	st7789_wr_cmd(dev, 0x36);
	if(dev->rotation==0)st7789_wr_data8(dev, 0x00);
	else if(dev->rotation==1)st7789_wr_data8(dev, 0xC0);
	else if(dev->rotation==2)st7789_wr_data8(dev, 0x70);
	else st7789_wr_data8(dev, 0xA0);

	st7789_wr_cmd(dev, 0x3A); 
	st7789_wr_data8(dev, 0x05);

	st7789_wr_cmd(dev, 0xB2);
	st7789_wr_data8(dev, 0x0C);
	st7789_wr_data8(dev, 0x0C);
	st7789_wr_data8(dev, 0x00);
	st7789_wr_data8(dev, 0x33);
	st7789_wr_data8(dev, 0x33); 

	st7789_wr_cmd(dev, 0xB7); 
	st7789_wr_data8(dev, 0x35);  

	st7789_wr_cmd(dev, 0xBB);
	st7789_wr_data8(dev, 0x19);

	st7789_wr_cmd(dev, 0xC0);
	st7789_wr_data8(dev, 0x2C);

	st7789_wr_cmd(dev, 0xC2);
	st7789_wr_data8(dev, 0x01);

	st7789_wr_cmd(dev, 0xC3);
	st7789_wr_data8(dev, 0x12);   

	st7789_wr_cmd(dev, 0xC4);
	st7789_wr_data8(dev, 0x20);  

	st7789_wr_cmd(dev, 0xC6); 
	st7789_wr_data8(dev, 0x0F);    

	st7789_wr_cmd(dev, 0xD0); 
	st7789_wr_data8(dev, 0xA4);
	st7789_wr_data8(dev, 0xA1);

	st7789_wr_cmd(dev, 0xE0);
	st7789_wr_data8(dev, 0xD0);
	st7789_wr_data8(dev, 0x04);
	st7789_wr_data8(dev, 0x0D);
	st7789_wr_data8(dev, 0x11);
	st7789_wr_data8(dev, 0x13);
	st7789_wr_data8(dev, 0x2B);
	st7789_wr_data8(dev, 0x3F);
	st7789_wr_data8(dev, 0x54);
	st7789_wr_data8(dev, 0x4C);
	st7789_wr_data8(dev, 0x18);
	st7789_wr_data8(dev, 0x0D);
	st7789_wr_data8(dev, 0x0B);
	st7789_wr_data8(dev, 0x1F);
	st7789_wr_data8(dev, 0x23);



	st7789_wr_cmd(dev, 0xE1);
	st7789_wr_data8(dev, 0xD0);
	st7789_wr_data8(dev, 0x04);
	st7789_wr_data8(dev, 0x0C);
	st7789_wr_data8(dev, 0x11);
	st7789_wr_data8(dev, 0x13);
	st7789_wr_data8(dev, 0x2C);
	st7789_wr_data8(dev, 0x3F);
	st7789_wr_data8(dev, 0x44);
	st7789_wr_data8(dev, 0x51);
	st7789_wr_data8(dev, 0x2F);
	st7789_wr_data8(dev, 0x1F);
	st7789_wr_data8(dev, 0x1F);
	st7789_wr_data8(dev, 0x20);
	st7789_wr_data8(dev, 0x23);
	st7789_wr_cmd(dev, 0x21); 

	st7789_wr_cmd(dev, 0x29); 
}

void st7789_addr_set(st7789_device_t* dev, uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend){
	uint16_t x_shift = dev->horizontal_offset;
	uint16_t y_shift = dev->vertical_offset;

	if(dev->rotation==0){
		st7789_wr_cmd(dev, 0x2a);
		st7789_wr_data8(dev, (xsta+x_shift)>>8);
		st7789_wr_data8(dev, xsta+x_shift);
		st7789_wr_data8(dev, (xend+x_shift)>>8);
		st7789_wr_data8(dev, xend+x_shift);
		st7789_wr_cmd(dev, 0x2b);
		st7789_wr_data8(dev, (ysta+y_shift)>>8);
		st7789_wr_data8(dev, ysta+y_shift);
		st7789_wr_data8(dev, (yend+y_shift)>>8);
		st7789_wr_data8(dev, yend+y_shift);
		st7789_wr_cmd(dev, 0x2c);
	}else if(dev->rotation==1){
		st7789_wr_cmd(dev, 0x2a);
		st7789_wr_data8(dev, (xsta+x_shift)>>8);
		st7789_wr_data8(dev, xsta+x_shift);
		st7789_wr_data8(dev, (xend+x_shift)>>8);
		st7789_wr_data8(dev, xend+x_shift);
		st7789_wr_cmd(dev, 0x2b);
		st7789_wr_data8(dev, (ysta+y_shift)>>8);
		st7789_wr_data8(dev, ysta+y_shift);
		st7789_wr_data8(dev, (yend+y_shift)>>8);
		st7789_wr_data8(dev, yend+y_shift);
		st7789_wr_cmd(dev, 0x2c);
	}else if(dev->rotation==2){
		st7789_wr_cmd(dev, 0x2a);
		st7789_wr_data8(dev, (xsta+x_shift)>>8);
		st7789_wr_data8(dev, xsta+x_shift);
		st7789_wr_data8(dev, (xend+x_shift)>>8);  
		st7789_wr_data8(dev, xend+x_shift);
		st7789_wr_cmd(dev, 0x2b);
		st7789_wr_data8(dev, (ysta+y_shift)>>8);
		st7789_wr_data8(dev, ysta+y_shift);
		st7789_wr_data8(dev, (yend+y_shift)>>8);
		st7789_wr_data8(dev, yend+y_shift);
		st7789_wr_cmd(dev, 0x2c);
	}else{
		st7789_wr_cmd(dev, 0x2a);
		st7789_wr_data8(dev, (xsta+x_shift)>>8);
		st7789_wr_data8(dev, xsta+x_shift);
		st7789_wr_data8(dev, (xend+x_shift)>>8);
		st7789_wr_data8(dev, xend+x_shift);
		st7789_wr_cmd(dev, 0x2b);
		st7789_wr_data8(dev, (ysta+y_shift)>>8);
		st7789_wr_data8(dev, ysta+y_shift);
		st7789_wr_data8(dev, (yend+y_shift)>>8);
		st7789_wr_data8(dev, yend+y_shift);
		st7789_wr_cmd(dev, 0x2c);
	}
}

void st7789_fill(st7789_device_t* dev, uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint32_t color){
  uint32_t color32 = color << 16 | color;
    st7789_addr_set(dev, xsta, ysta, xend - 1, yend - 1);
    // for (uint16_t i = ysta; i < yend; i++){ 
    //     // for (uint16_t j = xsta; j < xend; j += 64){
    //     //     st7789_wr_data32x32(dev, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, 
    //     //       color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32);
    //     // }
    //     for (uint16_t j = xsta; j < xend; j ++){
    //         st7789_wr_data16(dev, color);
    //     }
    // }
	for(uint16_t i = 0; i < (xend - xsta) * (yend - ysta) / 2; i+=32){
        st7789_wr_data32x32(dev, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, 
          color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32, color32);
    }
}

void st7789_fill_img(st7789_device_t* dev, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint32_t *data) {
  st7789_addr_set(dev, x, y, x + w - 1, y + h - 1);
  for (uint16_t i = 0; i < w * h / 2; i+=32) {
    st7789_wr_data32x32(dev, data[i],data[i+1],data[i+2],data[i+3],data[i+4],data[i+5],data[i+6],data[i+7],data[i+8],data[i+9],data[i+10],data[i+11],data[i+12],data[i+13],data[i+14],data[i+15],
        data[i+16],data[i+17],data[i+18],data[i+19],data[i+20],data[i+21],data[i+22],data[i+23],data[i+24],data[i+25],data[i+26],data[i+27],data[i+28],data[i+29],data[i+30],data[i+31]);
  }
}