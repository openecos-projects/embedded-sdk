#include "main.h"

/* @BSP_NAME@ 最小示例。 */
void main(void)
{
    /* [按需修改] 如果 BSP 不提供系统串口，请改成已经实现的最小 HAL 示例。 */
    hal_sys_uart_init();
    hal_sys_putstr("Hello from @BSP_NAME@!\n\r");

    /* [不要修改] 裸机 main 返回后的行为通常未定义，因此停在这里。 */
    for (;;) {
    }
}
