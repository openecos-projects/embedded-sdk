#ifndef ECOS_HAL_ADC_H
#define ECOS_HAL_ADC_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t hal_adc_id_t;

typedef struct {
    uint8_t channel;
    uint8_t sample_cycles;
    uint32_t clock_divider;
} hal_adc_config_t;

int hal_adc_get_instance_count(void);
ecos_err_t hal_adc_init(hal_adc_id_t adc, const hal_adc_config_t *config);
ecos_err_t hal_adc_deinit(hal_adc_id_t adc);
ecos_err_t hal_adc_read(hal_adc_id_t adc, uint16_t *value);

#ifdef __cplusplus
}
#endif

#endif
