/* CH32V103 ADC1 single-channel polling backend. */
#include "ecos/hal/adc.h"

#include <stddef.h>
#include <stdint.h>

#define REG16(address) (*(volatile uint16_t *)(uintptr_t)(address))
#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#define RCC_CFGR0 REG32(0x40021004u)
#define RCC_APB2PCENR REG32(0x40021018u)
#define GPIOA_CFGLR REG32(0x40010800u)
#define GPIOB_CFGLR REG32(0x40010C00u)
#define GPIOC_CFGLR REG32(0x40011000u)
#define ADC1_BASE 0x40012400u
#define ADC1_STATR REG32(ADC1_BASE + 0x00u)
#define ADC1_CTLR1 REG32(ADC1_BASE + 0x04u)
#define ADC1_CTLR2 REG32(ADC1_BASE + 0x08u)
#define ADC1_SAMPTR1 REG32(ADC1_BASE + 0x0Cu)
#define ADC1_SAMPTR2 REG32(ADC1_BASE + 0x10u)
#define ADC1_RSQR1 REG32(ADC1_BASE + 0x2Cu)
#define ADC1_RSQR2 REG32(ADC1_BASE + 0x30u)
#define ADC1_RSQR3 REG32(ADC1_BASE + 0x34u)
#define ADC1_RDATAR REG32(ADC1_BASE + 0x4Cu)
#define ADC_STATR_EOC 0x02u
#define ADC_CTLR2_ADON 0x01u
#define ADC_CTLR2_CAL 0x04u
#define ADC_CTLR2_RSTCAL 0x08u
#define ADC_CTLR2_EXTTRIG 0x00100000u
#define ADC_CTLR2_SWSTART 0x00400000u
#define ADC_CTLR2_TSVREFE 0x00800000u
#define ADC_TIMEOUT 200000u

static uint8_t initialized;
static uint8_t configured_channel;

static uint8_t sample_code(uint8_t cycles)
{
    if (cycles <= 2u) return 0u;   /* 1.5 cycles */
    if (cycles <= 8u) return 1u;   /* 7.5 */
    if (cycles <= 14u) return 2u;  /* 13.5 */
    if (cycles <= 29u) return 3u;  /* 28.5 */
    if (cycles <= 42u) return 4u;  /* 41.5 */
    if (cycles <= 56u) return 5u;  /* 55.5 */
    if (cycles <= 72u) return 6u;  /* 71.5 */
    return 7u;                     /* 239.5 */
}

static int valid_id(hal_adc_id_t adc)
{
    return adc == 0u;
}

static int valid_channel(uint8_t channel)
{
    return channel <= 17u;
}

static void configure_analog_pin(uint8_t channel)
{
    volatile uint32_t *reg = NULL;
    uint8_t pin = channel;

    if (channel <= 7u) {
        reg = &GPIOA_CFGLR;
    } else if (channel <= 9u) {
        reg = &GPIOB_CFGLR;
        pin = (uint8_t)(channel - 8u);
    } else if (channel <= 15u) {
        reg = &GPIOC_CFGLR;
        pin = (uint8_t)(channel - 10u);
    } else {
        return;
    }
    *reg &= ~(0xFu << ((uint32_t)pin * 4u)); /* analog input: MODE=00,CNF=00 */
}

static int wait_flag_clear(uint32_t mask)
{
    uint32_t timeout = ADC_TIMEOUT;

    while ((ADC1_CTLR2 & mask) != 0u) {
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    return ECOS_OK;
}

static int wait_eoc(void)
{
    uint32_t timeout = ADC_TIMEOUT;
    while ((ADC1_STATR & ADC_STATR_EOC) == 0u) {
        if (timeout-- == 0u)
            return ECOS_ERR_TIMEOUT;
    }
    return ECOS_OK;
}

int hal_adc_get_instance_count(void)
{
    return 1;
}

ecos_err_t hal_adc_init(hal_adc_id_t adc, const hal_adc_config_t *config)
{
    uint8_t code;
    uint32_t sample_bits;

    if (!valid_id(adc) || config == NULL || !valid_channel(config->channel) ||
        (config->clock_divider != 2u && config->clock_divider != 4u &&
         config->clock_divider != 6u && config->clock_divider != 8u))
        return ECOS_ERR_INVALID_ARGUMENT;
    code = sample_code(config->sample_cycles);
    RCC_APB2PCENR |= (1u << 9) | (1u << 2) | (1u << 3) | (1u << 4);
    configure_analog_pin(config->channel);
    RCC_CFGR0 = (RCC_CFGR0 & ~(3u << 14)) |
                (((config->clock_divider / 2u) - 1u) << 14);
    ADC1_CTLR1 = 0u;
    ADC1_SAMPTR1 = 0u;
    ADC1_SAMPTR2 = 0u;
    if (config->channel <= 9u)
        sample_bits = (uint32_t)code << ((uint32_t)config->channel * 3u);
    else if (config->channel <= 17u)
        sample_bits = (uint32_t)code << ((uint32_t)(config->channel - 10u) * 3u);
    else
        sample_bits = 0u;
    if (config->channel <= 9u)
        ADC1_SAMPTR2 = sample_bits;
    else if (config->channel <= 17u)
        ADC1_SAMPTR1 = sample_bits;
    ADC1_RSQR1 = 0u;
    ADC1_RSQR2 = 0u;
    ADC1_RSQR3 = config->channel;
    ADC1_CTLR2 = ADC_CTLR2_ADON | ADC_CTLR2_EXTTRIG;
    if (config->channel >= 16u)
        ADC1_CTLR2 |= ADC_CTLR2_TSVREFE;
    ADC1_CTLR2 |= ADC_CTLR2_RSTCAL;
    if (wait_flag_clear(ADC_CTLR2_RSTCAL) != ECOS_OK)
        return ECOS_ERR_TIMEOUT;
    ADC1_CTLR2 |= ADC_CTLR2_CAL;
    if (wait_flag_clear(ADC_CTLR2_CAL) != ECOS_OK)
        return ECOS_ERR_TIMEOUT;
    configured_channel = config->channel;
    initialized = 1u;
    return ECOS_OK;
}

ecos_err_t hal_adc_deinit(hal_adc_id_t adc)
{
    if (!valid_id(adc))
        return ECOS_ERR_INVALID_ARGUMENT;
    ADC1_CTLR2 = 0u;
    initialized = 0u;
    return ECOS_OK;
}

ecos_err_t hal_adc_read(hal_adc_id_t adc, uint16_t *value)
{
    int result;
    if (!valid_id(adc) || value == NULL)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (initialized == 0u)
        return ECOS_ERR_NOT_INITIALIZED;
    ADC1_RSQR3 = configured_channel;
    ADC1_CTLR2 |= ADC_CTLR2_SWSTART;
    result = wait_eoc();
    if (result != ECOS_OK)
        return (ecos_err_t)result;
    *value = (uint16_t)(ADC1_RDATAR & 0x0FFFu);
    return ECOS_OK;
}
