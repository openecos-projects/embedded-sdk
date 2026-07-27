#include "main.h"

void main(void){   
    hal_sys_uart_init();
    printf("ST7789 LCD TEST!\n");


    hal_qspi_config_t qspi_config = {
        .clkdiv = 1,
    };
    hal_qspi_init(HAL_QSPI_PORT_0, &qspi_config);

    st7789_device_t st7789 = {
        .dc_gpio_port = 0,
        .dc_gpio_pin = GPIO_NUM_0,
        .rst_gpio_port = 0,
        .rst_gpio_pin = GPIO_NUM_1,
        .qspi_port = HAL_QSPI_PORT_0,
        .qspi_cs = HAL_QSPI_CS_0,
        .screen_width = 240,
        .screen_height = 240,
        .rotation = 2,
        .horizontal_offset = 0,
        .vertical_offset = 0,
    };
    st7789_init(&st7789);
    st7789_fill(&st7789, 0, 0, 240, 240, 0xFFFFFFFF);
    st7789_fill(&st7789, 0, 0, 240, 240, 0x00000000);
    st7789_fill(&st7789, 0, 0, 240, 240, 0xAAAAAAAA);
}
