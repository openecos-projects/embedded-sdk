#ifndef __QSPI_TYPE_H__
#define __QSPI_TYPE_H__

#include <stdint.h>

typedef struct {
    uint32_t clkdiv; // QSPI时钟分频值
} qspi_config_t;

typedef enum {
    QSPI_CS_0 = 1 << 16,
    QSPI_CS_1 = 2 << 16,
    QSPI_CS_2 = 3 << 16,
    QSPI_CS_3 = 4 << 16,
} qspi_cs_t;

#endif