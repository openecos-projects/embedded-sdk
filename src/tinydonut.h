#ifndef TINYDONUT_H__
#define TINYDONUT_H__

#include <stdint.h>
#include "tinylcd.h"

// 灰度级别定义 (从暗到亮)
#define GRAY_LEVELS 12
extern const uint16_t gray_colors[GRAY_LEVELS];

// 屏幕缓冲区 (128x128像素)
#define scale_x 3
#define scale_y 5
#define DONUT_WIDTH  80 * scale_x
#define DONUT_HEIGHT 22 * scale_y
extern uint16_t donut_framebuffer[DONUT_WIDTH * DONUT_HEIGHT];

// TFT屏幕显示函数
void screen_clear(void);
void screen_refresh(void);
void print_pixel(uint16_t color, int x, int y);

// donut动画函数
void donut(void);

#endif // TINYDONUT_H__
