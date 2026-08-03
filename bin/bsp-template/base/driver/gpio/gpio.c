#include "hal_gpio.h"
#include "board.h"

/* @BSP_NAME@ GPIO HAL 实现。 */
/* [不要修改] 以下函数名称和参数由 SDK hal_gpio.h 定义。 */

void gpio_hal_input_enable(uint8_t gpio_id, uint8_t gpio_num)
{
    /* [必须修改] TODO_BSP_REQUIRED：根据实际寄存器设置输入模式。 */
    (void)gpio_id;
    (void)gpio_num;
}

void gpio_hal_output_enable(uint8_t gpio_id, uint8_t gpio_num)
{
    /* [必须修改] TODO_BSP_REQUIRED：根据实际寄存器设置输出模式。 */
    (void)gpio_id;
    (void)gpio_num;
}

void gpio_hal_set_level(uint8_t gpio_id, uint8_t gpio_num, uint8_t level)
{
    /* [必须修改] TODO_BSP_REQUIRED：设置指定引脚电平。 */
    (void)gpio_id;
    (void)gpio_num;
    (void)level;
}

uint8_t gpio_hal_get_level(uint8_t gpio_id, uint8_t gpio_num)
{
    /* [必须修改] TODO_BSP_REQUIRED：读取指定引脚电平。 */
    (void)gpio_id;
    (void)gpio_num;
    return 0;
}

void gpio_hal_read_update(void)
{
    /* [按需修改] 硬件不需要显式刷新时可保留空实现。 */
}

void gpio_hal_write_update(void)
{
    /* [按需修改] 硬件不需要显式刷新时可保留空实现。 */
}

void gpio_hal_set_fcfg(uint8_t gpio_id, uint8_t gpio_num, uint8_t val)
{
    /* [按需修改] 芯片有功能选择寄存器时在这里实现。 */
    (void)gpio_id;
    (void)gpio_num;
    (void)val;
}

void gpio_hal_set_mux(uint8_t gpio_id, uint8_t gpio_num, uint8_t val)
{
    /* [按需修改] 芯片有引脚复用寄存器时在这里实现。 */
    (void)gpio_id;
    (void)gpio_num;
    (void)val;
}
