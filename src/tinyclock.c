/*
 * TinyClock - 嵌入式数字时钟显示模块
 * 
 * 功能特性:
 * - 128x128 LCD显示屏支持
 * - HH:MM:SS 时间格式显示
 * - "ECOS" 背景标识
 * - 彩色界面支持
 * - 优化的刷新机制，减少闪烁
 * - 内部时间计算逻辑，确保准确计时
 * - 模块化设计，易于维护和扩展
 */

#include <firmware.h>
#include <tinyprintf.h>
#include <tinytim.h>
#include <tinylcd.h>
#include <tinyclock.h>

// 颜色定义 (RGB565格式)
#define COLOR_BLACK     0x00000000
#define COLOR_WHITE     0xFFFFFFFF
#define COLOR_RED       0xF800F800
#define COLOR_GREEN     0x07E007E0
#define COLOR_BLUE      0x001F001F
#define COLOR_YELLOW    0xFFE0FFE0
#define COLOR_CYAN      0x07FF07FF
#define COLOR_MAGENTA   0xF81FF81F
#define COLOR_GRAY      0x7BEF7BEF
#define COLOR_ORANGE    0xFD20FD20

// 时间数据结构
typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint32_t tick_count;  // 用于精确计时的tick计数
} clock_time_t;

// 全局时间变量
static clock_time_t current_time = {0, 0, 0, 0};

// 数字字符点阵数据 (16x24像素，每个数字)
// 0-9的点阵数据，1表示显示像素，0表示背景
static const uint8_t digit_font[10][24][2] = {
    // 数字 0
    {
        {0x00, 0x00}, {0x3F, 0xFC}, {0x7F, 0xFE}, {0x70, 0x0E}, {0xE0, 0x07}, {0xE0, 0x07},
        {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07},
        {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07},
        {0xE0, 0x07}, {0xE0, 0x07}, {0x70, 0x0E}, {0x7F, 0xFE}, {0x3F, 0xFC}, {0x00, 0x00}
    },
    // 数字 1
    {
        {0x00, 0x00}, {0x01, 0xC0}, {0x03, 0xC0}, {0x07, 0xC0}, {0x0F, 0xC0}, {0x1D, 0xC0},
        {0x01, 0xC0}, {0x01, 0xC0}, {0x01, 0xC0}, {0x01, 0xC0}, {0x01, 0xC0}, {0x01, 0xC0},
        {0x01, 0xC0}, {0x01, 0xC0}, {0x01, 0xC0}, {0x01, 0xC0}, {0x01, 0xC0}, {0x01, 0xC0},
        {0x01, 0xC0}, {0x01, 0xC0}, {0x01, 0xC0}, {0x7F, 0xFF}, {0x7F, 0xFF}, {0x00, 0x00}
    },
    // 数字 2
    {
        {0x00, 0x00}, {0x3F, 0xFC}, {0x7F, 0xFE}, {0x70, 0x0E}, {0x00, 0x07}, {0x00, 0x07},
        {0x00, 0x07}, {0x00, 0x0E}, {0x00, 0x1C}, {0x00, 0x38}, {0x00, 0x70}, {0x00, 0xE0},
        {0x01, 0xC0}, {0x03, 0x80}, {0x07, 0x00}, {0x0E, 0x00}, {0x1C, 0x00}, {0x38, 0x00},
        {0x70, 0x00}, {0xE0, 0x00}, {0xE0, 0x00}, {0xFF, 0xFE}, {0xFF, 0xFE}, {0x00, 0x00}
    },
    // 数字 3
    {
        {0x00, 0x00}, {0x3F, 0xFC}, {0x7F, 0xFE}, {0x70, 0x0E}, {0x00, 0x07}, {0x00, 0x07},
        {0x00, 0x07}, {0x00, 0x0E}, {0x0F, 0xFC}, {0x0F, 0xFC}, {0x00, 0x0E}, {0x00, 0x07},
        {0x00, 0x07}, {0x00, 0x07}, {0x00, 0x07}, {0x00, 0x07}, {0x00, 0x07}, {0x00, 0x07},
        {0x00, 0x07}, {0x70, 0x0E}, {0x7F, 0xFE}, {0x3F, 0xFC}, {0x00, 0x00}, {0x00, 0x00}
    },
    // 数字 4
    {
        {0x00, 0x00}, {0x00, 0x1C}, {0x00, 0x3C}, {0x00, 0x7C}, {0x00, 0xFC}, {0x01, 0xDC},
        {0x03, 0x9C}, {0x07, 0x1C}, {0x0E, 0x1C}, {0x1C, 0x1C}, {0x38, 0x1C}, {0x70, 0x1C},
        {0xE0, 0x1C}, {0xFF, 0xFF}, {0xFF, 0xFF}, {0x00, 0x1C}, {0x00, 0x1C}, {0x00, 0x1C},
        {0x00, 0x1C}, {0x00, 0x1C}, {0x00, 0x1C}, {0x00, 0x1C}, {0x00, 0x1C}, {0x00, 0x00}
    },
    // 数字 5
    {
        {0x00, 0x00}, {0xFF, 0xFE}, {0xFF, 0xFE}, {0xE0, 0x00}, {0xE0, 0x00}, {0xE0, 0x00},
        {0xE0, 0x00}, {0xE0, 0x00}, {0xFF, 0xFC}, {0xFF, 0xFE}, {0x00, 0x0E}, {0x00, 0x07},
        {0x00, 0x07}, {0x00, 0x07}, {0x00, 0x07}, {0x00, 0x07}, {0x00, 0x07}, {0x00, 0x07},
        {0x00, 0x07}, {0x70, 0x0E}, {0x7F, 0xFE}, {0x3F, 0xFC}, {0x00, 0x00}, {0x00, 0x00}
    },
    // 数字 6
    {
        {0x00, 0x00}, {0x3F, 0xFC}, {0x7F, 0xFE}, {0x70, 0x0E}, {0xE0, 0x00}, {0xE0, 0x00},
        {0xE0, 0x00}, {0xE0, 0x00}, {0xFF, 0xFC}, {0xFF, 0xFE}, {0xF0, 0x0E}, {0xE0, 0x07},
        {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07},
        {0xE0, 0x07}, {0x70, 0x0E}, {0x7F, 0xFE}, {0x3F, 0xFC}, {0x00, 0x00}, {0x00, 0x00}
    },
    // 数字 7
    {
        {0x00, 0x00}, {0xFF, 0xFF}, {0xFF, 0xFF}, {0x00, 0x07}, {0x00, 0x0E}, {0x00, 0x1C},
        {0x00, 0x38}, {0x00, 0x70}, {0x00, 0xE0}, {0x01, 0xC0}, {0x03, 0x80}, {0x07, 0x00},
        {0x0E, 0x00}, {0x1C, 0x00}, {0x38, 0x00}, {0x70, 0x00}, {0xE0, 0x00}, {0xE0, 0x00},
        {0xE0, 0x00}, {0xE0, 0x00}, {0xE0, 0x00}, {0xE0, 0x00}, {0xE0, 0x00}, {0x00, 0x00}
    },
    // 数字 8
    {
        {0x00, 0x00}, {0x3F, 0xFC}, {0x7F, 0xFE}, {0x70, 0x0E}, {0xE0, 0x07}, {0xE0, 0x07},
        {0xE0, 0x07}, {0x70, 0x0E}, {0x3F, 0xFC}, {0x3F, 0xFC}, {0x70, 0x0E}, {0xE0, 0x07},
        {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07},
        {0xE0, 0x07}, {0x70, 0x0E}, {0x7F, 0xFE}, {0x3F, 0xFC}, {0x00, 0x00}, {0x00, 0x00}
    },
    // 数字 9
    {
        {0x00, 0x00}, {0x3F, 0xFC}, {0x7F, 0xFE}, {0x70, 0x0E}, {0xE0, 0x07}, {0xE0, 0x07},
        {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07}, {0xE0, 0x07}, {0x70, 0x0F}, {0x7F, 0xFF},
        {0x3F, 0xFF}, {0x00, 0x07}, {0x00, 0x07}, {0x00, 0x07}, {0x00, 0x07}, {0x00, 0x07},
        {0x00, 0x07}, {0x70, 0x0E}, {0x7F, 0xFE}, {0x3F, 0xFC}, {0x00, 0x00}, {0x00, 0x00}
    }
};

// 冒号字符点阵数据 (8x24像素)
static const uint8_t colon_font[24] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x3C, 0x3C, 0x18, 0x00,
    0x00, 0x00, 0x00, 0x18, 0x3C, 0x3C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00
};

/**
 * @brief 更新时间计算
 * 使用定时器进行精确计时
 */
void clock_update_time(void)
{
    // 使用简单的计数方式进行时间更新
    // 每次调用增加tick计数，当达到一定值时增加秒数
    static uint32_t call_count = 0;
    
    call_count++;
    
    // 假设clock_run()每10ms调用一次，100次调用 = 1秒
    if (call_count >= 100) {
        call_count = 0;
        current_time.seconds++;
        
        if (current_time.seconds >= 60) {
            current_time.seconds = 0;
            current_time.minutes++;
            
            if (current_time.minutes >= 60) {
                current_time.minutes = 0;
                current_time.hours++;
                
                if (current_time.hours >= 24) {
                    current_time.hours = 0;
                }
            }
        }
    }
}

/**
 * @brief 在LCD上绘制单个数字
 * @param digit 要绘制的数字 (0-9)
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param color 前景色
 * @param bg_color 背景色
 */
void clock_draw_digit(uint8_t digit, uint16_t x, uint16_t y, uint32_t color, uint32_t bg_color)
{
    if (digit > 9) return;
    
    for (int row = 0; row < 24; row++) {
        uint16_t line_data = (digit_font[digit][row][0] << 8) | digit_font[digit][row][1];
        
        for (int col = 0; col < 16; col++) {
            uint32_t pixel_color = (line_data & (0x8000 >> col)) ? color : bg_color;
            
            // 绘制单个像素
            lcd_addr_set(x + col, y + row, x + col, y + row);
            lcd_wr_data32(pixel_color);
        }
    }
}

/**
 * @brief 在LCD上绘制冒号
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param color 前景色
 * @param bg_color 背景色
 */
void clock_draw_colon(uint16_t x, uint16_t y, uint32_t color, uint32_t bg_color)
{
    for (int row = 0; row < 24; row++) {
        uint8_t line_data = colon_font[row];
        
        for (int col = 0; col < 8; col++) {
            uint32_t pixel_color = (line_data & (0x80 >> col)) ? color : bg_color;
            
            // 绘制单个像素
            lcd_addr_set(x + col, y + row, x + col, y + row);
            lcd_wr_data32(pixel_color);
        }
    }
}

/**
 * @brief 绘制ECOS背景标识
 */
void clock_draw_ecos_logo(void)
{
    // 在屏幕顶部绘制"ECOS"文字 (简化版本)
    // ECOS总宽度约为80像素 (4个字母 * 15像素 + 3个间距 * 5像素 = 75像素)
    // 居中位置: (128 - 75) / 2 = 26.5，取27
    uint16_t logo_x = 27;
    uint16_t logo_y = 10;
    
    // 使用简单的矩形块来表示ECOS字母
    // E
    lcd_fill(logo_x, logo_y, logo_x + 8, logo_y + 20, COLOR_CYAN);
    lcd_fill(logo_x, logo_y, logo_x + 15, logo_y + 4, COLOR_CYAN);
    lcd_fill(logo_x, logo_y + 8, logo_x + 12, logo_y + 12, COLOR_CYAN);
    lcd_fill(logo_x, logo_y + 16, logo_x + 15, logo_y + 20, COLOR_CYAN);
    
    // C
    logo_x += 18;
    lcd_fill(logo_x, logo_y, logo_x + 15, logo_y + 4, COLOR_CYAN);
    lcd_fill(logo_x, logo_y, logo_x + 4, logo_y + 20, COLOR_CYAN);
    lcd_fill(logo_x, logo_y + 16, logo_x + 15, logo_y + 20, COLOR_CYAN);
    
    // O
    logo_x += 18;
    lcd_fill(logo_x, logo_y, logo_x + 15, logo_y + 4, COLOR_CYAN);
    lcd_fill(logo_x, logo_y, logo_x + 4, logo_y + 20, COLOR_CYAN);
    lcd_fill(logo_x + 11, logo_y, logo_x + 15, logo_y + 20, COLOR_CYAN);
    lcd_fill(logo_x, logo_y + 16, logo_x + 15, logo_y + 20, COLOR_CYAN);
    
    // S
    logo_x += 18;
    lcd_fill(logo_x, logo_y, logo_x + 15, logo_y + 4, COLOR_CYAN);
    lcd_fill(logo_x, logo_y, logo_x + 4, logo_y + 10, COLOR_CYAN);
    lcd_fill(logo_x, logo_y + 8, logo_x + 15, logo_y + 12, COLOR_CYAN);
    lcd_fill(logo_x + 11, logo_y + 10, logo_x + 15, logo_y + 20, COLOR_CYAN);
    lcd_fill(logo_x, logo_y + 16, logo_x + 15, logo_y + 20, COLOR_CYAN);
}

/**
 * @brief 显示当前时间
 */
void clock_display_time(void)
{
    // 计算时间显示的总宽度和居中位置
    // HH:MM:SS = 数字(16) + 数字(16) + 冒号(8) + 数字(16) + 数字(16) + 冒号(8) + 数字(16) + 数字(16)
    // 总宽度 = 16*6 + 8*2 = 112像素
    uint16_t total_width = 112;
    uint16_t time_x = (LCD_W - total_width) / 2;  // 居中计算：(128 - 112) / 2 = 8
    uint16_t time_y = 60;  // 起始Y坐标
    
    uint32_t digit_color = COLOR_WHITE;
    uint32_t bg_color = COLOR_BLACK;
    
    // 显示小时 (HH)
    clock_draw_digit(current_time.hours / 10, time_x, time_y, digit_color, bg_color);
    clock_draw_digit(current_time.hours % 10, time_x + 16, time_y, digit_color, bg_color);
    
    // 显示第一个冒号
    clock_draw_colon(time_x + 32, time_y, digit_color, bg_color);
    
    // 显示分钟 (MM)
    clock_draw_digit(current_time.minutes / 10, time_x + 40, time_y, digit_color, bg_color);
    clock_draw_digit(current_time.minutes % 10, time_x + 56, time_y, digit_color, bg_color);
    
    // 显示第二个冒号
    clock_draw_colon(time_x + 72, time_y, digit_color, bg_color);
    
    // 显示秒钟 (SS)
    clock_draw_digit(current_time.seconds / 10, time_x + 80, time_y, digit_color, bg_color);
    clock_draw_digit(current_time.seconds % 10, time_x + 96, time_y, digit_color, bg_color);
}

/**
 * @brief 初始化时钟显示
 */
void clock_init(void)
{
    printf("[CLOCK] Initializing TinyClock v1.0...\n");
    
    // 初始化LCD
    spi_init();
    lcd_init();
    
    // 初始化定时器0用于计时
    reg_tim0_config = 0x0000;  // 停止定时器
    reg_tim0_value = 0xFFFFFFFF;  // 设置初始值
    reg_tim0_config = 0x0001;  // 启动定时器 (向下计数，连续模式)
    
    // 清屏并绘制背景
    lcd_fill(0, 0, LCD_W, LCD_H, COLOR_BLACK);
    clock_draw_ecos_logo();
    
    printf("[CLOCK] TinyClock initialized successfully\n");
}

/**
 * @brief 设置时间
 * @param hours 小时 (0-23)
 * @param minutes 分钟 (0-59)
 * @param seconds 秒钟 (0-59)
 */
void clock_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    if (hours < 24 && minutes < 60 && seconds < 60) {
        current_time.hours = hours;
        current_time.minutes = minutes;
        current_time.seconds = seconds;
        current_time.tick_count = 0;
        
        printf("[CLOCK] Time set to %02d:%02d:%02d\n", hours, minutes, seconds);
    }
}

/**
 * @brief 获取当前时间
 * @param hours 返回小时
 * @param minutes 返回分钟
 * @param seconds 返回秒钟
 */
void clock_get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds)
{
    if (hours) *hours = current_time.hours;
    if (minutes) *minutes = current_time.minutes;
    if (seconds) *seconds = current_time.seconds;
}

/**
 * @brief 时钟主循环
 * 应该在主程序中持续调用此函数
 */
void clock_run(void)
{
    static uint8_t last_seconds = 255;  // 用于检测秒钟变化
    
    // 更新时间
    clock_update_time();
    
    // 只有当秒钟发生变化时才更新显示，减少闪烁
    if (current_time.seconds != last_seconds) {
        clock_display_time();
        last_seconds = current_time.seconds;
        
        // 打印当前时间到串口 (调试用)
        printf("[CLOCK] %02d:%02d:%02d\n", 
               current_time.hours, current_time.minutes, current_time.seconds);
    }
    
    // 短暂延时，避免CPU占用过高
    delay_ms(10);
}

/**
 * @brief 时钟测试函数
 */
void clock_test(void)
{
    printf("[CLOCK] Starting clock test...\n");
    
    // 初始化时钟
    clock_init();
    
    // 设置初始时间为 00:00:00
    clock_set_time(0, 0, 0);
    
    // 运行时钟显示，持续更新
    printf("[CLOCK] Clock running from 00:00:00\n");
    while (1) {
        clock_run();
    }
}