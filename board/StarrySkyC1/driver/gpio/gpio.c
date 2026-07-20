#include "gpio.h"
#include "hal_gpio.h"

void gpio_hal_input_enable(uint8_t gpio_id, uint8_t gpio_num) {
    (void)gpio_id;
    gpio_set_direction((gpio_num_t)gpio_num, GPIO_MODE_INPUT);
}

void gpio_hal_output_enable(uint8_t gpio_id, uint8_t gpio_num) {
    (void)gpio_id;
    gpio_set_direction((gpio_num_t)gpio_num, GPIO_MODE_OUTPUT);
}

void gpio_hal_set_level(uint8_t gpio_id, uint8_t gpio_num, uint8_t level) {
    (void)gpio_id;
    gpio_set_level((gpio_num_t)gpio_num, (gpio_level_t)level);
}

uint8_t gpio_hal_get_level(uint8_t gpio_id, uint8_t gpio_num) {
    (void)gpio_id;
    return (uint8_t)gpio_get_level((gpio_num_t)gpio_num);
}

void gpio_hal_read_update(void) {
}

void gpio_hal_write_update(void) {
}

void gpio_hal_set_fcfg(uint8_t gpio_id, uint8_t gpio_num, uint8_t val) {
    (void)gpio_id;
    (void)gpio_num;
    (void)val;
}

void gpio_hal_set_mux(uint8_t gpio_id, uint8_t gpio_num, uint8_t val) {
    (void)gpio_id;
    (void)gpio_num;
    (void)val;
}
