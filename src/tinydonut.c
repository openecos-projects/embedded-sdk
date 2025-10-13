/**
 * Donut动画移植到128x128 TFT屏幕
 * 
 * 基于原始donut.c代码，适配到TFT屏幕显示
 * 使用灰度像素替代ASCII字符
 */

#include <stdint.h>
#include <string.h>
#include "tinylcd.h"
#include "tinydonut.h"

// 灰度颜色映射 (RGB565格式，从暗到亮)
const uint16_t gray_colors[GRAY_LEVELS] = {
    0x0000,  // 黑色 (对应 ' ')
    0x1082,  // 很暗的灰色 (对应 '.')
    0x2104,  // 暗灰色 (对应 ',')
    0x3186,  // 较暗灰色 (对应 '-')
    0x4208,  // 中暗灰色 (对应 '~')
    0x528A,  // 中灰色 (对应 ':')
    0x630C,  // 中亮灰色 (对应 ';')
    0x738E,  // 较亮灰色 (对应 '=')
    0x8410,  // 亮灰色 (对应 '!')
    0x9492,  // 很亮灰色 (对应 '*')
    0xA514,  // 极亮灰色 (对应 '#')
    0xB596   // 最亮灰色 (对应 '$')
};

// 屏幕缓冲区
uint16_t donut_framebuffer[DONUT_WIDTH * DONUT_HEIGHT];

// 清空屏幕缓冲区
void screen_clear(void) {
    memset(donut_framebuffer, 0x00, sizeof(donut_framebuffer));
}

// 刷新屏幕显示
void screen_refresh(void) {
    // 使用新的图像绘制函数
    lcd_draw_image(donut_framebuffer, 0, 0, DONUT_WIDTH, DONUT_HEIGHT);
}

// 在指定位置绘制字符对应的灰度像素
void print_pixel(uint16_t color, int screen_x, int screen_y) {
    // 确保坐标在原始80x22范围内
    if (screen_x >= 0 && screen_x < 80 && 
        screen_y >= 0 && screen_y < 22) {
        
        // 计算缩放后的起始位置
        int scaled_x = screen_x * scale_x;
        int scaled_y = screen_y * scale_y;
        
        // 绘制缩放后的像素块
        for(int i = 0; i < scale_x; i++) {
            for(int j = 0; j < scale_y; j++) {
                int pixel_x = scaled_x + i;
                int pixel_y = scaled_y + j;
                
                // 确保不超出屏幕边界
                if (pixel_x < DONUT_WIDTH && pixel_y < DONUT_HEIGHT) {
                    int pixel_index = pixel_y * DONUT_WIDTH + pixel_x;
                    donut_framebuffer[pixel_index] = color;
                }
            }
        }
    }
}
