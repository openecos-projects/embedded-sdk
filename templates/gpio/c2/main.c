#include "main.h"

void main(void){
    
    sys_uart_init();
    printf("GPIO test\n");

    gpio_config_t gpio_config_out = {
        .pin_bit_mask = (1ULL << GPIO_NUM_0),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&gpio_config_out);

    gpio_config_t gpio_config_in = {
        .pin_bit_mask = (1ULL << GPIO_NUM_1),
        .mode = GPIO_MODE_INPUT,
    };
    gpio_config(&gpio_config_in);

    while(1){
        if(gpio_get_level(GPIO_NUM_1) == GPIO_LEVEL_HIGH){
            gpio_set_level(GPIO_NUM_0, GPIO_LEVEL_HIGH);
        }
        else{
            gpio_set_level(GPIO_NUM_0, GPIO_LEVEL_LOW);
        }
    }


}