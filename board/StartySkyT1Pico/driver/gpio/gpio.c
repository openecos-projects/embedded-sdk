#include <stdint.h>

#include "hal_gpio.h"
#include "board.h"

/* 保存四组 GPIO 端口的数据、方向和输入寄存器地址。 */
static volatile uint32_t *const gpio_data_registers[STARTYSKY_T1_PICO_GPIO_PORT_COUNT] =
{
    &REG_GPIO_0_SWPORTA_DR,
    &REG_GPIO_0_SWPORTB_DR,
    &REG_GPIO_0_SWPORTC_DR,
    &REG_GPIO_0_SWPORTD_DR
};

static volatile uint32_t *const gpio_direction_registers[STARTYSKY_T1_PICO_GPIO_PORT_COUNT] =
{
    &REG_GPIO_0_SWPORTA_DDR,
    &REG_GPIO_0_SWPORTB_DDR,
    &REG_GPIO_0_SWPORTC_DDR,
    &REG_GPIO_0_SWPORTD_DDR
};

static volatile uint32_t *const gpio_input_registers[STARTYSKY_T1_PICO_GPIO_PORT_COUNT] =
{
    &REG_GPIO_0_EXT_PORTA,
    &REG_GPIO_0_EXT_PORTB,
    &REG_GPIO_0_EXT_PORTC,
    &REG_GPIO_0_EXT_PORTD
};

/**
 * 判断 GPIO 端口和引脚编号是否位于 StartySky T1-Pico 有效范围内。
 */
static uint8_t startysky_t1_pico_gpio_is_valid(uint8_t gpio_id, uint8_t gpio_num)
{
    /* 同时检查四组端口和每组低八位引脚范围。 */
    return (uint8_t)((gpio_id < STARTYSKY_T1_PICO_GPIO_PORT_COUNT) &&
                     (gpio_num < STARTYSKY_T1_PICO_GPIO_PINS_PER_PORT));
}


/**
 * 将指定 GPIO 引脚配置为输入。
 */
void gpio_hal_input_enable(uint8_t gpio_id, uint8_t gpio_num)
{
    uint32_t direction;

    /* 忽略超出 StartySky T1-Pico GPIO 范围的配置请求。 */
    if (startysky_t1_pico_gpio_is_valid(gpio_id, gpio_num) == 0u)
        return;

    /* 清除对应方向位，使引脚工作在输入模式。 */
    direction = *gpio_direction_registers[gpio_id];
    direction &= ~(1u << gpio_num);
    *gpio_direction_registers[gpio_id] = direction;
}


/**
 * 将指定 GPIO 引脚配置为输出。
 */
void gpio_hal_output_enable(uint8_t gpio_id, uint8_t gpio_num)
{
    uint32_t direction;

    /* 忽略超出 StartySky T1-Pico GPIO 范围的配置请求。 */
    if (startysky_t1_pico_gpio_is_valid(gpio_id, gpio_num) == 0u)
        return;

    /* 设置对应方向位，使引脚工作在输出模式。 */
    direction = *gpio_direction_registers[gpio_id];
    direction |= 1u << gpio_num;
    *gpio_direction_registers[gpio_id] = direction;
}


/**
 * 设置指定 GPIO 输出引脚的逻辑电平。
 */
void gpio_hal_set_level(uint8_t gpio_id, uint8_t gpio_num, uint8_t level)
{
    uint32_t output;

    /* 忽略超出 StartySky T1-Pico GPIO 范围的输出请求。 */
    if (startysky_t1_pico_gpio_is_valid(gpio_id, gpio_num) == 0u)
        return;

    /* 读取端口输出值并更新指定引脚。 */
    output = *gpio_data_registers[gpio_id];
    if (level == 0u)
        output &= ~(1u << gpio_num);
    else
        output |= 1u << gpio_num;

    /* 将更新后的完整端口值写回数据寄存器。 */
    *gpio_data_registers[gpio_id] = output;
}


/**
 * 读取指定 GPIO 引脚的实际输入电平。
 */
uint8_t gpio_hal_get_level(uint8_t gpio_id, uint8_t gpio_num)
{
    /* 对无效端口或引脚统一返回低电平。 */
    if (startysky_t1_pico_gpio_is_valid(gpio_id, gpio_num) == 0u)
        return 0u;

    /* 从外部端口寄存器提取指定引脚电平。 */
    return (uint8_t)((*gpio_input_registers[gpio_id] >> gpio_num) & 1u);
}


/**
 * 完成 GPIO 输入值刷新操作。
 */
void gpio_hal_read_update(void)
{
    /* 输入寄存器实时反映引脚状态，不需要软件刷新。 */
}


/**
 * 完成 GPIO 输出值刷新操作。
 */
void gpio_hal_write_update(void)
{
    /* 数据寄存器写入后立即生效，不需要软件刷新。 */
}


/**
 * 配置指定 GPIO 引脚的功能属性。
 */
void gpio_hal_set_fcfg(uint8_t gpio_id, uint8_t gpio_num, uint8_t val)
{
    /* 已验证资料未提供功能配置寄存器，因此不执行写操作。 */
    (void)gpio_id;
    (void)gpio_num;
    (void)val;
}


/**
 * 配置指定 GPIO 引脚的复用功能。
 */
void gpio_hal_set_mux(uint8_t gpio_id, uint8_t gpio_num, uint8_t val)
{
    /* 已验证资料未提供引脚复用寄存器，因此不执行写操作。 */
    (void)gpio_id;
    (void)gpio_num;
    (void)val;
}
