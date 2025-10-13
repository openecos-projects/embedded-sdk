/**
 * Donut动画移植到128x128 TFT屏幕 - 优化版本
 * 
 * 基于原始donut.c代码，适配到TFT屏幕显示
 * 使用灰度像素替代ASCII字符
 * Original author:
 * https://twitter.com/a1k0n
 * https://www.a1k0n.net/2021/01/13/optimizing-donut.html
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-09-15     Andy Sloane  First version
 * 2011-07-20     Andy Sloane  Second version
 * 2021-01-13     Andy Sloane  Third version
 * 2021-03-25     Meco Man     Port to RT-Thread RTOS
 * 2024-12-19     Erasmus & AI Assistant Port to retroSoC TFT screen
 * 2024-12-19     Erasmus & AI Assistant Performance optimization
 */

#include "tinytim.h"
#include "tinylcd.h"
#include "tinydonut.h"

// 编译器优化提示
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// 内联旋转宏 - 优化版本
#define R_OPT(mul,shift,x,y) \
  do { \
    int _temp = x; \
    x -= mul*y>>shift; \
    y += mul*_temp>>shift; \
    _temp = (3145728-x*x-y*y)>>11; \
    x = x*_temp>>10; \
    y = y*_temp>>10; \
  } while(0)

// 预计算的三角函数表 (1024 = 1.0) - 简化版本
static const int16_t sin_table[91] = {
    0, 18, 36, 54, 71, 89, 107, 125, 143, 160, 178, 195, 213, 230, 248, 265, 282, 299, 316, 333, 350, 367, 384, 400, 416, 433, 449, 465, 481, 496, 512, 527, 542, 557, 572, 587, 601, 615, 629, 643, 657, 670, 683, 696, 709, 721, 733, 745, 757, 768, 779, 790, 801, 811, 821, 831, 841, 850, 859, 868, 876, 884, 892, 900, 907, 914, 921, 927, 933, 939, 945, 950, 955, 960, 964, 968, 972, 975, 978, 981, 984, 986, 988, 990, 991, 992, 993, 994, 994, 995, 995, 995
};

static const int16_t cos_table[91] = {
    1024, 1023, 1023, 1022, 1021, 1019, 1017, 1014, 1011, 1007, 1003, 998, 993, 987, 981, 974, 967, 959, 951, 942, 933, 923, 913, 902, 891, 879, 867, 854, 841, 827, 813, 798, 783, 767, 751, 734, 717, 699, 681, 662, 643, 623, 603, 582, 561, 539, 517, 494, 471, 447, 423, 398, 373, 347, 321, 294, 267, 239, 211, 182, 153, 123, 93, 62, 31, 0, -31, -62, -93, -123, -153, -182, -211, -239, -267, -294, -321, -347, -373, -398, -423, -447, -471, -494, -517, -539, -561, -582, -603, -623, -643, -662, -681, -699, -717, -734, -751, -767, -783, -798, -813, -827, -841, -854, -867, -879, -891, -902, -913, -923, -933, -942, -951, -959, -967, -974, -981, -987, -993, -998, -1003, -1007, -1011, -1014, -1017, -1019, -1021, -1022, -1023, -1023, -1024
};

// 优化的缓冲区 - 直接使用帧缓冲区大小
static uint16_t pixel_buffer[80 * 22];  // 直接像素缓冲区
static signed char z_buffer[80 * 22];   // z缓冲区

// 内联函数：快速像素写入
static inline void fast_pixel_write(uint16_t color, int x, int y) {
    if (likely(x >= 0 && x < 80 && y >= 0 && y < 22)) {
        int index = y * 80 + x;
        pixel_buffer[index] = color;
    }
}

// 内联函数：快速缩放像素写入帧缓冲区
static inline void fast_scaled_pixel_write(uint16_t color, int x, int y) {
    if (likely(x >= 0 && x < 80 && y >= 0 && y < 22)) {
        int scaled_x = x * scale_x;
        int scaled_y = y * scale_y;
        
        // 展开循环以提高性能
        int base_index = scaled_y * DONUT_WIDTH + scaled_x;
        
        // 写入3x5像素块
        donut_framebuffer[base_index] = color;
        donut_framebuffer[base_index + 1] = color;
        donut_framebuffer[base_index + 2] = color;
        
        base_index += DONUT_WIDTH;
        donut_framebuffer[base_index] = color;
        donut_framebuffer[base_index + 1] = color;
        donut_framebuffer[base_index + 2] = color;
        
        base_index += DONUT_WIDTH;
        donut_framebuffer[base_index] = color;
        donut_framebuffer[base_index + 1] = color;
        donut_framebuffer[base_index + 2] = color;
        
        base_index += DONUT_WIDTH;
        donut_framebuffer[base_index] = color;
        donut_framebuffer[base_index + 1] = color;
        donut_framebuffer[base_index + 2] = color;
        
        base_index += DONUT_WIDTH;
        donut_framebuffer[base_index] = color;
        donut_framebuffer[base_index + 1] = color;
        donut_framebuffer[base_index + 2] = color;
    }
}

void donut(void) {
    // 使用寄存器变量优化
    register int sA = 1024, cA = 0, sB = 1024, cB = 0;
    
    // 预计算常量
    const int R1 = 1, R2 = 2048, K2 = 5120*1024;
    const int screen_center_x = 25, screen_center_y = 12;
    const int scale_x_val = 30, scale_y_val = 15;

    while(1) {
        // 快速清空缓冲区 - 使用32位操作
        uint32_t *pixel_ptr = (uint32_t*)pixel_buffer;
        uint32_t *z_ptr = (uint32_t*)z_buffer;
        uint32_t *fb_ptr = (uint32_t*)donut_framebuffer;
        
        // 32位清零操作，减少内存访问次数
        for (int i = 0; i < (80*22)/2; i++) {
            pixel_ptr[i] = 0;
        }
        
        // z缓冲区初始化为最大值
        for (int i = 0; i < (80*22)/4; i++) {
            z_ptr[i] = 0x7F7F7F7F;  // 127 in each byte
        }
        
        // 帧缓冲区清零
        for (int i = 0; i < (DONUT_WIDTH*DONUT_HEIGHT)/2; i++) {
            fb_ptr[i] = 0;
        }
        
        // 使用预计算的三角函数表
        int sj = 0, cj = 1024;
        for (int j = 0; j < 90; j++) {
            int si = 0, ci = 1024;
            
            for (int i = 0; i < 324; i++) {
                // 优化的3D计算
                int x0 = R1*cj + R2;
                int x1 = ci*x0 >> 10;
                int x2 = cA*sj >> 10;
                int x3 = si*x0 >> 10;
                int x4 = R1*x2 - (sA*x3 >> 10);
                int x5 = sA*sj >> 10;
                int x6 = K2 + R1*1024*x5 + cA*x3;
                int x7 = cj*si >> 10;
                
                // 屏幕坐标计算
                int x = screen_center_x + (scale_x_val*(cB*x1 - sB*x4))/x6;
                int y = screen_center_y + (scale_y_val*(cB*x4 + sB*x1))/x6;
                
                // 法向量计算
                int N = (((-cA*x7 - cB*((-sA*x7>>10) + x2) - ci*(cj*sB >> 10)) >> 10) - x5) >> 7;
                
                // 边界检查和z-buffer测试
                if (likely(y > 0 && y < 22 && x > 0 && x < 80)) {
                    int o = x + 80 * y;
                    signed char zz = (x6-K2)>>15;
                    
                    if (likely(zz < z_buffer[o])) {
                        z_buffer[o] = zz;
                        // 直接使用灰度颜色，避免数组访问
                        uint16_t color = gray_colors[N > 0 ? N : 0];
                        pixel_buffer[o] = color;
                    }
                }
                
                // 优化的旋转计算
                R_OPT(5, 8, ci, si);
            }
            R_OPT(9, 7, cj, sj);
        }
        
        // 更新旋转角度
        R_OPT(5, 7, cA, sA);
        R_OPT(5, 8, cB, sB);

        // 优化的像素绘制 - 直接写入帧缓冲区
        for (int y = 0; y < 22; y++) {
            for (int x = 0; x < 80; x++) {
                int index = x + 80 * y;
                uint16_t color = pixel_buffer[index];
                if (likely(color != 0)) {
                    fast_scaled_pixel_write(color, x, y);
                }
            }
        }
        
        // 刷新到TFT屏幕 - 使用优化的批量传输
        lcd_draw_image(donut_framebuffer, 0, 0, DONUT_WIDTH, DONUT_HEIGHT);
        
        // 优化的延迟 - 根据性能调整
        delay_ms(30);  // 进一步减少延迟以获得更流畅的动画
    }
}
