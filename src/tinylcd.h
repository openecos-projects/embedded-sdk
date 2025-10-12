#ifndef TINYLCD_H__
#define TINYLCD_H__

#include <stdint.h>

// GPIO控制宏定义
#define lcd_dc_clr     (reg_gpio_data = (uint32_t)0b000)
#define lcd_dc_set     (reg_gpio_data = (uint32_t)0b100)

// 显示方向设置
#define USE_HORIZONTAL 2

// LCD尺寸定义
#if USE_HORIZONTAL == 0 || USE_HORIZONTAL == 1
#define LCD_W 128
#define LCD_H 128
#else
#define LCD_W 128
#define LCD_H 128
#endif

// SPI命令定义
#define SPI_CMD_RD     0
#define SPI_CMD_WR     1
#define SPI_CMD_QRD    2
#define SPI_CMD_QWR    3
#define SPI_CMD_SWRST  4

// SPI片选定义
#define SPI_CSN0       0
#define SPI_CSN1       1
#define SPI_CSN2       2
#define SPI_CSN3       3

// 核心函数声明
void spi_init(void);
void spi_wr_dat(uint8_t dat);
void lcd_wr_cmd(uint8_t cmd);
void lcd_wr_data8(uint8_t dat);
void lcd_wr_data16(uint16_t dat);
void lcd_wr_data32(uint32_t dat);
void lcd_init(void);
void lcd_addr_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint32_t color);

// 图像绘制函数声明
void lcd_draw_image(const uint16_t *data, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void lcd_draw_image_8bit(const uint8_t *data, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void lcd_draw_image_32bit(const uint32_t *data, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

// 测试函数声明
void ip_lcd_test(void);

#endif // TINYLCD_H__