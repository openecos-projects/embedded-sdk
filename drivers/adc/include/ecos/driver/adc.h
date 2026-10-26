#ifndef ECOS_DRIVER_ADC_H
#define ECOS_DRIVER_ADC_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ecos_adc_id_t;
#define ECOS_ADC_DEFAULT ((ecos_adc_id_t)0u)

typedef struct {
    uint8_t channel;
    uint8_t sample_cycles;
    uint32_t clock_divider;
} ecos_adc_config_t;

#define ECOS_ADC_CONFIG_DEFAULT \
    { 0u, 239u, 6u }

int ecos_adc_get_instance_count(void);
ecos_err_t ecos_adc_init(ecos_adc_id_t adc, const ecos_adc_config_t *config);
ecos_err_t ecos_adc_deinit(ecos_adc_id_t adc);
ecos_err_t ecos_adc_read(ecos_adc_id_t adc, uint16_t *value);

#ifdef __cplusplus
}
#endif

#endif
