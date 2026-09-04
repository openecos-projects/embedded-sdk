#include "main.h"

/* 定义演示使用的 GPIO 端口和引脚数量。 */
#define GPIO_DEMO_PORT              0u
#define GPIO_DEMO_PIN_COUNT         8u
#define GPIO_DEMO_DELAY_LOOPS       ((uint32_t)CONFIG_CPU_FREQ_MHZ * 100000u)

/**
 * 使用软件循环产生可观察的延时。
 */
static void gpio_demo_delay(void)
{
    volatile uint32_t count;

    /* 循环时间受主频和编译器影响，不用于精确计时。 */
    for (count = 0u; count < GPIO_DEMO_DELAY_LOOPS; ++count)
        __asm__ volatile("nop");
}


/**
 * 设置 GPIOA 低八位的输出电平。
 */
static void gpio_demo_set_port(uint8_t level)
{
    uint8_t pin;

    /* 逐一更新 GPIOA 低八位并完成输出刷新。 */
    for (pin = 0u; pin < GPIO_DEMO_PIN_COUNT; ++pin)
        gpio_hal_set_level(GPIO_DEMO_PORT, pin, level);

    gpio_hal_write_update();
}


/**
 * 初始化 GPIOA 低八位为低电平输出。
 */
static void gpio_demo_init(void)
{
    uint8_t pin;

    /* 先设置低电平，再启用输出以避免产生短暂高电平。 */
    gpio_demo_set_port(GPIO_LEVEL_LOW);
    for (pin = 0u; pin < GPIO_DEMO_PIN_COUNT; ++pin)
        gpio_hal_output_enable(GPIO_DEMO_PORT, pin);
}


/**
 * 初始化串口和 GPIO，然后周期翻转 GPIOA 低八位。
 */
int main(void)
{
    uint8_t level = GPIO_LEVEL_LOW;

    /* 初始化串口和 GPIO 演示引脚。 */
    hal_sys_uart_init();
    gpio_demo_init();
    hal_sys_putstr("StartySky T1-Pico GPIO polling demo started.\n");

    /* 使用软件延时周期翻转 GPIOA 低八位。 */
    for (;;)
    {
        gpio_demo_delay();
        level = (level == GPIO_LEVEL_LOW) ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;
        gpio_demo_set_port(level);

        if (level == GPIO_LEVEL_HIGH)
            hal_sys_putstr("GPIOA[7:0] HIGH.\n");
        else
            hal_sys_putstr("GPIOA[7:0] LOW.\n");
    }
}
