/*
 * TinyClock - 嵌入式数字时钟显示模块头文件
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

#ifndef __TINYCLOCK_H__
#define __TINYCLOCK_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化时钟显示
 * 
 * 此函数会初始化LCD显示屏、定时器和相关硬件，
 * 清屏并绘制ECOS背景标识
 */
void clock_init(void);

/**
 * @brief 设置时间
 * @param hours 小时 (0-23)
 * @param minutes 分钟 (0-59)
 * @param seconds 秒钟 (0-59)
 * 
 * 设置当前时间，参数会进行有效性检查
 */
void clock_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds);

/**
 * @brief 获取当前时间
 * @param hours 返回小时 (可为NULL)
 * @param minutes 返回分钟 (可为NULL)
 * @param seconds 返回秒钟 (可为NULL)
 * 
 * 获取当前时间，任何参数都可以为NULL
 */
void clock_get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);

/**
 * @brief 时钟主循环
 * 
 * 应该在主程序中持续调用此函数，
 * 负责时间更新和显示刷新，
 * 内置优化机制减少不必要的屏幕刷新
 */
void clock_run(void);

/**
 * @brief 时钟测试函数
 * 
 * 执行完整的时钟功能测试，
 * 包括初始化、时间设置和显示测试
 */
void clock_test(void);

/**
 * @brief 更新时间计算 (内部函数)
 * 
 * 使用定时器进行精确计时，
 * 自动处理秒、分、时的进位
 */
void clock_update_time(void);

/**
 * @brief 显示当前时间 (内部函数)
 * 
 * 在LCD上以HH:MM:SS格式显示当前时间
 */
void clock_display_time(void);

/**
 * @brief 绘制ECOS背景标识 (内部函数)
 * 
 * 在屏幕顶部绘制"ECOS"文字标识
 */
void clock_draw_ecos_logo(void);

/**
 * @brief 在LCD上绘制单个数字 (内部函数)
 * @param digit 要绘制的数字 (0-9)
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param color 前景色
 * @param bg_color 背景色
 */
void clock_draw_digit(uint8_t digit, uint16_t x, uint16_t y, uint32_t color, uint32_t bg_color);

/**
 * @brief 在LCD上绘制冒号 (内部函数)
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param color 前景色
 * @param bg_color 背景色
 */
void clock_draw_colon(uint16_t x, uint16_t y, uint32_t color, uint32_t bg_color);

#ifdef __cplusplus
}
#endif

#endif /* __TINYCLOCK_H__ */