#ifndef __MYSPI_H
#define __MYSPI_H

#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "hal_gpio.h"

void MYSPI_Init(void);
void MYSPI_Start(void);
void MYSPI_Stop(void);
uint8_t MYSPI_SwapByte(uint8_t wdata);

#define SS_PORT 0
#define SS_PIN GPIO_NUM_0
#define CLK_PORT 0
#define CLK_PIN GPIO_NUM_1
#define MOSI_PORT 0
#define MOSI_PIN GPIO_NUM_2
#define MISO_PORT 0
#define MISO_PIN GPIO_NUM_3

#endif