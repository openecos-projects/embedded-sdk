#include "MySPI.h"

static void MYSPI_W_SS(uint8_t BitValue){
    gpio_hal_set_level(SS_PORT,SS_PIN,BitValue);
}

static void MYSPI_W_CLK(uint8_t BitValue){
    gpio_hal_set_level(CLK_PORT,CLK_PIN,BitValue);
}

static void MYSPI_W_MOSI(uint8_t BitValue){
    gpio_hal_set_level(MOSI_PORT,MOSI_PIN,BitValue);
}

static uint8_t MYSPI_R_MISO(){
	return gpio_hal_get_level(MISO_PORT,MISO_PIN);
}

void MYSPI_Init(){
    gpio_hal_output_enable(SS_PORT,SS_PIN);
    gpio_hal_output_enable(CLK_PORT,CLK_PIN);
    gpio_hal_output_enable(MOSI_PORT,MOSI_PIN);
    gpio_hal_input_enable(MISO_PORT,MISO_PIN);

	MYSPI_W_SS(1);
	MYSPI_W_CLK(0);
    gpio_hal_write_update();
}

void MYSPI_Start(){
	MYSPI_W_SS(0);
    gpio_hal_write_update();
}

void MYSPI_Stop(){
	MYSPI_W_SS(1);
    gpio_hal_write_update();
}

uint8_t MYSPI_SwapByte(uint8_t wdata){
	uint8_t rdata = 0x00;
	for(int i=0;i<8;i++){
		MYSPI_W_MOSI((wdata>>i)&0x01); // 使用左移可能会出现bug
        gpio_hal_write_update();

        MYSPI_W_CLK(1);
        gpio_hal_write_update();

        gpio_hal_read_update();
        if(MYSPI_R_MISO() == 1) rdata |= (0x80>>i);
        
		MYSPI_W_CLK(0);
	}
    gpio_hal_write_update();
	return rdata;
}