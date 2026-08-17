#include <stddef.h>
#include <stdint.h>

#include "hal_gpio.h"
#include "hal_interrupt.h"
#include "board.h"

static gpio_hal_isr_t gpio_handlers[STARTYSKY_T1_GPIO_PINS_PER_PORT];
static void *gpio_handler_args[STARTYSKY_T1_GPIO_PINS_PER_PORT];

/**
 * 判断 GPIO 中断端口和引脚是否由当前硬件支持。
 */
static uint8_t startysky_t1_gpio_intr_is_valid(uint8_t gpio_id,
                                               uint8_t gpio_num)
{
    /* 当前仅 GPIO port A 的八个引脚连接到独立 PLIC source。 */
    return (uint8_t)((gpio_id == STARTYSKY_T1_GPIO_PORT_A) &&
                     (gpio_num < STARTYSKY_T1_GPIO_PINS_PER_PORT));
}


/**
 * 将 GPIO port A 引脚编号转换为 PLIC 中断源编号。
 */
static uint32_t startysky_t1_gpio_intr_source(uint8_t gpio_num)
{
    /* GPIOA0 至 GPIOA7 依次映射到 PLIC source 8 至 15。 */
    return STARTYSKY_T1_GPIOA_PLIC_SOURCE_BASE + gpio_num;
}


/**
 * 清除 GPIO 锁存状态并调用对应引脚的用户处理函数。
 */
static void startysky_t1_gpio_intr_dispatch(void *arg)
{
    uint8_t gpio_num = (uint8_t)(uintptr_t)arg;
    uint32_t mask = 1u << gpio_num;
    gpio_hal_isr_t handler;

    /* 在调用用户处理函数前清除本引脚锁存的 GPIO 中断。 */
    REG_GPIO_0_PORTA_EOI = mask;
    __asm__ volatile("fence iorw, iorw" : : : "memory");

    /* 使用稳定快照调用当前引脚注册的用户处理函数。 */
    handler = gpio_handlers[gpio_num];
    if (handler != NULL)
        handler(gpio_handler_args[gpio_num]);
}


/**
 * 配置 GPIO port A 引脚的中断触发类型。
 */
int gpio_hal_set_intr_type(uint8_t gpio_id,
                           uint8_t gpio_num,
                           gpio_intr_type_t intr_type)
{
    uint32_t mask;

    /* 拒绝无独立 PLIC source 的端口、引脚和未知触发类型。 */
    if ((startysky_t1_gpio_intr_is_valid(gpio_id, gpio_num) == 0u) ||
        (intr_type > GPIO_INTR_HIGH_LEVEL))
        return -1;

    /* 关闭类型只禁止本引脚中断，不修改其余配置位。 */
    if (intr_type == GPIO_INTR_DISABLE)
        return gpio_hal_intr_disable(gpio_id, gpio_num);

    /* 将目标引脚设置为软件控制的输入模式并关闭去抖。 */
    mask = 1u << gpio_num;
    REG_GPIO_0_SWPORTA_CTL &= ~mask;
    REG_GPIO_0_SWPORTA_DDR &= ~mask;
    REG_GPIO_0_DEBOUNCE &= ~mask;

    /* 先清除双边沿位，再按请求配置边沿或电平触发。 */
    REG_GPIO_0_INT_BOTHEDGE &= ~mask;
    if ((intr_type == GPIO_INTR_POSEDGE) ||
        (intr_type == GPIO_INTR_NEGEDGE) ||
        (intr_type == GPIO_INTR_ANYEDGE))
        REG_GPIO_0_INTTYPE_LEVEL |= mask;
    else
        REG_GPIO_0_INTTYPE_LEVEL &= ~mask;

    /* 配置触发极性，并为双边沿模式设置专用控制位。 */
    if ((intr_type == GPIO_INTR_POSEDGE) ||
        (intr_type == GPIO_INTR_HIGH_LEVEL))
        REG_GPIO_0_INT_POLARITY |= mask;
    else
        REG_GPIO_0_INT_POLARITY &= ~mask;

    if (intr_type == GPIO_INTR_ANYEDGE)
        REG_GPIO_0_INT_BOTHEDGE |= mask;

    /* 清除配置期间可能锁存的历史边沿。 */
    REG_GPIO_0_PORTA_EOI = mask;
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return 0;
}


/**
 * 为 GPIO port A 指定引脚注册中断处理函数。
 */
int gpio_hal_isr_handler_add(uint8_t gpio_id,
                             uint8_t gpio_num,
                             gpio_hal_isr_t handler,
                             void *arg,
                             uint32_t priority)
{
    uint32_t source;

    /* 拒绝无效引脚、空处理函数和重复注册。 */
    if ((startysky_t1_gpio_intr_is_valid(gpio_id, gpio_num) == 0u) ||
        (handler == NULL) || (gpio_handlers[gpio_num] != NULL))
        return -1;

    /* 先保存用户处理信息，再分配对应 PLIC source。 */
    gpio_handler_args[gpio_num] = arg;
    gpio_handlers[gpio_num] = handler;
    source = startysky_t1_gpio_intr_source(gpio_num);
    if (hal_intr_alloc(source,
                       priority,
                       startysky_t1_gpio_intr_dispatch,
                       (void *)(uintptr_t)gpio_num) != 0)
    {
        gpio_handlers[gpio_num] = NULL;
        gpio_handler_args[gpio_num] = NULL;
        return -1;
    }

    /* 注册阶段保持 GPIO 和 PLIC source 禁止，等待显式使能。 */
    return 0;
}


/**
 * 删除 GPIO port A 指定引脚的中断处理函数。
 */
int gpio_hal_isr_handler_remove(uint8_t gpio_id, uint8_t gpio_num)
{
    uint32_t source;

    /* 拒绝无效引脚或尚未注册的处理函数。 */
    if ((startysky_t1_gpio_intr_is_valid(gpio_id, gpio_num) == 0u) ||
        (gpio_handlers[gpio_num] == NULL))
        return -1;

    /* 先禁止外设和 PLIC source，再清除用户处理信息。 */
    (void)gpio_hal_intr_disable(gpio_id, gpio_num);
    source = startysky_t1_gpio_intr_source(gpio_num);
    if (hal_intr_free(source) != 0)
        return -1;

    gpio_handlers[gpio_num] = NULL;
    gpio_handler_args[gpio_num] = NULL;
    return 0;
}


/**
 * 使能 GPIO port A 指定引脚及其 PLIC source。
 */
int gpio_hal_intr_enable(uint8_t gpio_id, uint8_t gpio_num)
{
    uint32_t mask;
    uint32_t source;

    /* 只允许使能已经注册处理函数的有效引脚。 */
    if ((startysky_t1_gpio_intr_is_valid(gpio_id, gpio_num) == 0u) ||
        (gpio_handlers[gpio_num] == NULL))
        return -1;

    /* 清除历史状态并开启 GPIO 引脚中断。 */
    mask = 1u << gpio_num;
    REG_GPIO_0_PORTA_EOI = mask;
    REG_GPIO_0_INTEN |= mask;
    REG_GPIO_0_INTMASK &= ~mask;

    /* 最后开启对应 PLIC source，失败时恢复 GPIO 屏蔽。 */
    source = startysky_t1_gpio_intr_source(gpio_num);
    if (hal_intr_enable(source) != 0)
    {
        REG_GPIO_0_INTMASK |= mask;
        REG_GPIO_0_INTEN &= ~mask;
        return -1;
    }

    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return 0;
}


/**
 * 禁止 GPIO port A 指定引脚及其 PLIC source。
 */
int gpio_hal_intr_disable(uint8_t gpio_id, uint8_t gpio_num)
{
    uint32_t mask;
    uint32_t source;

    /* 拒绝无独立 PLIC source 的端口或引脚。 */
    if (startysky_t1_gpio_intr_is_valid(gpio_id, gpio_num) == 0u)
        return -1;

    /* 先屏蔽并禁止 GPIO 引脚，再清除锁存状态。 */
    mask = 1u << gpio_num;
    REG_GPIO_0_INTMASK |= mask;
    REG_GPIO_0_INTEN &= ~mask;
    REG_GPIO_0_PORTA_EOI = mask;

    /* 禁止对应 PLIC source 并同步寄存器访问。 */
    source = startysky_t1_gpio_intr_source(gpio_num);
    (void)hal_intr_disable(source);
    __asm__ volatile("fence iorw, iorw" : : : "memory");
    return 0;
}
