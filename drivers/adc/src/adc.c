#include "ecos/driver/adc.h"

#include "ecos/hal/adc.h"

#include <stddef.h>

int ecos_adc_get_instance_count(void)
{
    return hal_adc_get_instance_count();
}

ecos_err_t ecos_adc_init(ecos_adc_id_t adc, const ecos_adc_config_t *config)
{
    if (config == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    return hal_adc_init(adc, (const hal_adc_config_t *)config);
}

ecos_err_t ecos_adc_deinit(ecos_adc_id_t adc)
{
    return hal_adc_deinit(adc);
}

ecos_err_t ecos_adc_read(ecos_adc_id_t adc, uint16_t *value)
{
    return hal_adc_read(adc, value);
}
