#include "main.h"

void main(void){   
    sys_uart_init();
    printf("ST7789 LCD TEST!\n");

    qspi_config_t qspi_config = {
        .clkdiv = 1,
    };
    qspi_init(&qspi_config);

    st7789_device_t st7789 = {
        .dc_pin = GPIO_NUM_0,
        .rst_pin = GPIO_NUM_1,
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