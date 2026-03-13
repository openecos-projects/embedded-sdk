#include "main.h"
#include "board_init.h"

void main(void){
    
    // Initialize board-specific drivers
    starrysky_c2_board_init();
    
    // Get HAL driver instances
    sys_uart_driver_t* sys_uart_drv = sys_uart_get_driver();
    gpio_driver_t* gpio_drv = gpio_get_driver();

    sys_uart_drv->puts("GPIO test\n");

    gpio_config_t gpio_config_out = {
        .pin_bit_mask = (1ULL << GPIO_PIN_0),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_drv->config(&gpio_config_out);

    gpio_config_t gpio_config_in = {
        .pin_bit_mask = (1ULL << GPIO_PIN_1),
        .mode = GPIO_MODE_INPUT,
    };
    gpio_drv->config(&gpio_config_in);

    while(1){
        gpio_level_t level;
        gpio_drv->get_level(GPIO_PIN_1, &level);
        if(level == GPIO_LEVEL_HIGH){
            gpio_drv->set_level(GPIO_PIN_0, GPIO_LEVEL_HIGH);
        }
        else{
            gpio_drv->set_level(GPIO_PIN_0, GPIO_LEVEL_LOW);
        }
    }
}