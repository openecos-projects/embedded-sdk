#include "main.h"

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define GRAY_LEVELS 12
// 屏幕缓冲区 (128x128像素)
#define scale_x 8
#define scale_y 8
#define DONUT_SIZE 16
#define DONUT_WIDTH  DONUT_SIZE * scale_x
#define DONUT_HEIGHT DONUT_SIZE * scale_y

const uint32_t gray_colors[GRAY_LEVELS] = {
    0x00000000,  // 黑色 (对应 ' ')
    0x10821082,  // 很暗的灰色 (对应 '.')
    0x21042104,  // 暗灰色 (对应 ',')
    0x31863186,  // 较暗灰色 (对应 '-')
    0x42084208,  // 中暗灰色 (对应 '~')
    0x528A528A,  // 中灰色 (对应 ':')
    0x630C630C,  // 中亮灰色 (对应 ';')
    0x738E738E,  // 较亮灰色 (对应 '=')
    0x84108410,  // 亮灰色 (对应 '!')
    0x94929492,  // 很亮灰色 (对应 '*')
    0xA514A514,  // 极亮灰色 (对应 '#')
    0xB596B596   // 最亮灰色 (对应 '$')
};

uint32_t donut_framebuffer_32[DONUT_WIDTH * DONUT_HEIGHT];

void screen_clear(void) {
    memset(donut_framebuffer_32, 0x00, sizeof(donut_framebuffer_32));
}

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

// 预计算的三角函数表 (1024 = 1.0) - 仅保留91个值
static const int16_t sin_table[91] = {
    0, 18, 36, 54, 71, 89, 107, 125, 143, 160, 178, 195, 213, 230, 248, 265, 282, 299, 316, 333, 350, 367, 384, 400, 416, 433, 449, 465, 481, 496, 512, 527, 542, 557, 572, 587, 601, 615, 629, 643, 657, 670, 683, 696, 709, 721, 733, 745, 757, 768, 779, 790, 801, 811, 821, 831, 841, 850, 859, 868, 876, 884, 892, 900, 907, 914, 921, 927, 933, 939, 945, 950, 955, 960, 964, 968, 972, 975, 978, 981, 984, 986, 988, 990, 991, 992, 993, 994, 994, 995, 995
};

static const int16_t cos_table[91] = {
    1024, 1023, 1023, 1022, 1021, 1019, 1017, 1014, 1011, 1007, 1003, 998, 993, 987, 981, 974, 967, 959, 951, 942, 933, 923, 913, 902, 891, 879, 867, 854, 841, 827, 813, 798, 783, 767, 751, 734, 717, 699, 681, 662, 643, 623, 603, 582, 561, 539, 517, 494, 471, 447, 423, 398, 373, 347, 321, 294, 267, 239, 211, 182, 153, 123, 93, 62, 31, 0, -31, -62, -93, -123, -153, -182, -211, -239, -267, -294, -321, -347, -373, -398, -423, -447, -471, -494, -517, -539, -561, -582, -603, -623, -643
};

    // 优化的缓冲区 - 直接使用帧缓冲区大小
static uint32_t pixel_buffer[DONUT_SIZE * DONUT_SIZE];  // 直接像素缓冲区
static signed char z_buffer[DONUT_SIZE * DONUT_SIZE];   // z缓冲区

// 内联函数：快速像素写入
static inline void fast_pixel_write(uint32_t color, int x, int y) {
    if (likely(x >= 0 && x < DONUT_SIZE && y >= 0 && y < DONUT_SIZE)) {
        int index = y * DONUT_SIZE + x;
        pixel_buffer[index] = color;
    }
}

// 内联函数：快速缩放像素写入帧缓冲区
static inline void fast_scaled_pixel_write(uint32_t color, int x, int y) {
    if (likely(x >= 0 && x < DONUT_SIZE && y >= 0 && y < DONUT_SIZE)) {
        int scaled_x = x * scale_x;
        int scaled_y = y * scale_y;
        
        int base_index = scaled_y * DONUT_WIDTH + scaled_x;
        
        int row_index = base_index;
        donut_framebuffer_32[row_index/2] = color;
        donut_framebuffer_32[row_index/2 + 1] = color;
        donut_framebuffer_32[row_index/2 + 2] = color;
        donut_framebuffer_32[row_index/2 + 3] = color;
        
        row_index += DONUT_WIDTH;
        donut_framebuffer_32[row_index/2] = color;
        donut_framebuffer_32[row_index/2 + 1] = color;
        donut_framebuffer_32[row_index/2 + 2] = color;
        donut_framebuffer_32[row_index/2 + 3] = color;
        
        row_index += DONUT_WIDTH;
        donut_framebuffer_32[row_index/2] = color;
        donut_framebuffer_32[row_index/2 + 1] = color;
        donut_framebuffer_32[row_index/2 + 2] = color;
        donut_framebuffer_32[row_index/2 + 3] = color;
        
        row_index += DONUT_WIDTH;
        donut_framebuffer_32[row_index/2] = color;
        donut_framebuffer_32[row_index/2 + 1] = color;
        donut_framebuffer_32[row_index/2 + 2] = color;
        donut_framebuffer_32[row_index/2 + 3] = color;

        row_index += DONUT_WIDTH;
        donut_framebuffer_32[row_index/2] = color;
        donut_framebuffer_32[row_index/2 + 1] = color;
        donut_framebuffer_32[row_index/2 + 2] = color;
        donut_framebuffer_32[row_index/2 + 3] = color;

        row_index += DONUT_WIDTH;
        donut_framebuffer_32[row_index/2] = color;
        donut_framebuffer_32[row_index/2 + 1] = color;
        donut_framebuffer_32[row_index/2 + 2] = color;
        donut_framebuffer_32[row_index/2 + 3] = color;

        row_index += DONUT_WIDTH;
        donut_framebuffer_32[row_index/2] = color;
        donut_framebuffer_32[row_index/2 + 1] = color;
        donut_framebuffer_32[row_index/2 + 2] = color;
        donut_framebuffer_32[row_index/2 + 3] = color;

        row_index += DONUT_WIDTH;
        donut_framebuffer_32[row_index/2] = color;
        donut_framebuffer_32[row_index/2 + 1] = color;
        donut_framebuffer_32[row_index/2 + 2] = color;
        donut_framebuffer_32[row_index/2 + 3] = color;

    }
}

void donut(st7735_device_t *st7735) {
    // 使用寄存器变量优化
    register int sA = 1024, cA = 0, sB = 1024, cB = 0;
    
    // 预计算常量 - 优化版本
    const int R1 = 1, R2 = 1024, K2 = 4096*1024;  // 简化计算
    const int screen_center_x = DONUT_SIZE/2, screen_center_y = DONUT_SIZE/2;
    const int scale_x_val = 12, scale_y_val = 12;  // 使用2的幂次

    while(1) {
        
        // 初始化 83ms
        
        // 快速清空缓冲区 - 使用32位操作
        uint32_t *pixel_ptr = (uint32_t*)pixel_buffer;
        uint32_t *z_ptr = (uint32_t*)z_buffer;
        uint32_t *fb_ptr = (uint32_t*)donut_framebuffer_32;
        
        // 32位清零操作，减少内存访问次数
        for (int i = 0; i < (DONUT_SIZE*DONUT_SIZE)/2; i++) {
            pixel_ptr[i] = 0;
        }
        
        // z缓冲区初始化为最大值
        for (int i = 0; i < (DONUT_SIZE*DONUT_SIZE)/4; i++) {
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
            
            for (int i = 0; i < DONUT_SIZE*DONUT_SIZE; i++) {
                
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
                int x = screen_center_x + (scale_x_val*(cB*x1 - sB*x4)) / x6;
                int y = screen_center_y + (scale_y_val*(cB*x4 + sB*x1)) / x6;
                
                // 法向量计算
                int N = (((-cA*x7 - cB*((-sA*x7>>10) + x2) - ci*(cj*sB >> 10)) >> 10) - x5) >> 7;
                
                // 边界检查和z-buffer测试
                if (likely(y > 0 && y < DONUT_SIZE && x > 0 && x < DONUT_SIZE)) {

                    int o = x + DONUT_SIZE * y;
                    signed char zz = (x6-K2)>>15;
                    if (likely(zz < z_buffer[o])) {
                        z_buffer[o] = zz;
                        // 直接使用灰度颜色，避免数组访问
                        uint32_t color = gray_colors[N > 0 ? N : 0];
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
        // 优化的像素绘制 - 直接写入帧缓冲区 88ms
        for (int y = 0; y < DONUT_SIZE; y++) {
            for (int x = 0; x < DONUT_SIZE; x++) {
                int index = x + DONUT_SIZE * y;
                uint32_t color = pixel_buffer[index];
                if (likely(color != 0)) {
                    fast_scaled_pixel_write(color, x, y);
                }
            }
        }
        
        // 刷新到TFT屏幕 157ms
        st7735_fill_img(st7735,  0, 0, DONUT_WIDTH, DONUT_HEIGHT, donut_framebuffer_32);
    }
}

void main(void){
    sys_uart_init();
    printf("donut test!\n");

    qspi_config_t qspi_config = {
        .clkdiv = 0,
    };
    qspi_init(&qspi_config);

    st7735_device_t st7735 = {
        .dc_pin = GPIO_NUM_2,
        .screen_width = 128,
        .screen_height = 128,
        .rotation = 0,
        .horizontal_offset = 2,
        .vertical_offset = 3,
    };
    st7735_init(&st7735);
    
    donut(&st7735);
}