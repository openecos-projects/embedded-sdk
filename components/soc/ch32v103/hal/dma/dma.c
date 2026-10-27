/* CH32V103 DMA1 channel backend.
 * The C8T6 target exposes DMA1 channels 1..7.  DMA2 is deliberately not
 * claimed here: package/resource confirmation is required before exposing it.
 */
#include "ecos/hal/dma.h"
#include "ecos/hal/interrupt.h"
#include "ch32v103_interrupt_internal.h"

#include <stddef.h>
#include <stdint.h>

#define REG16(address) (*(volatile uint16_t *)(uintptr_t)(address))
#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))

#define RCC_AHBPCENR       REG32(0x40021014u)
#define DMA1_BASE          0x40020000u
#define DMA1_INTFR         REG32(DMA1_BASE + 0x00u)
#define DMA1_INTFCR        REG32(DMA1_BASE + 0x04u)
#define DMA1_CHANNEL_BASE  (DMA1_BASE + 0x08u)
#define DMA_CHANNEL_STRIDE 0x14u
#define DMA_CFGR(channel)  REG32(DMA1_CHANNEL_BASE + (uintptr_t)(channel) * DMA_CHANNEL_STRIDE + 0x00u)
#define DMA_CNTR(channel)  REG32(DMA1_CHANNEL_BASE + (uintptr_t)(channel) * DMA_CHANNEL_STRIDE + 0x04u)
#define DMA_PADDR(channel) REG32(DMA1_CHANNEL_BASE + (uintptr_t)(channel) * DMA_CHANNEL_STRIDE + 0x08u)
#define DMA_MADDR(channel) REG32(DMA1_CHANNEL_BASE + (uintptr_t)(channel) * DMA_CHANNEL_STRIDE + 0x0Cu)

#define DMA_CFGR_EN        0x0001u
#define DMA_CFGR_TCIE      0x0002u
#define DMA_CFGR_HTIE      0x0004u
#define DMA_CFGR_TEIE      0x0008u
#define DMA_CFGR_DIR       0x0010u
#define DMA_CFGR_CIRC      0x0020u
#define DMA_CFGR_PINC      0x0040u
#define DMA_CFGR_MINC      0x0080u
#define DMA_CFGR_PSIZE     0x0300u
#define DMA_CFGR_MSIZE     0x0C00u
#define DMA_CFGR_PL        0x3000u
#define DMA_CFGR_MEM2MEM   0x4000u

#define DMA_FLAG(channel, bit) (1u << ((uint32_t)(channel) * 4u + (bit)))
#define DMA_IRQ(channel) (27u + (uint32_t)(channel))

typedef struct {
    uint8_t initialized;
    uint8_t callback_enabled;
    uint32_t interrupt_mask;
    hal_dma_callback_t callback;
    void *arg;
} dma_state_t;

static dma_state_t states[7];

static int valid_channel(hal_dma_channel_t channel)
{
    return channel < 7u;
}

static int valid_width(hal_dma_width_t width)
{
    return width <= HAL_DMA_WIDTH_WORD;
}

static uint32_t width_bits(hal_dma_width_t width, uint32_t shift)
{
    return (uint32_t)width << shift;
}

static uint32_t flags_for(hal_dma_channel_t channel)
{
    return DMA_FLAG(channel, 0u) | DMA_FLAG(channel, 1u) |
           DMA_FLAG(channel, 2u) | DMA_FLAG(channel, 3u);
}

static uint32_t flag_value(hal_dma_channel_t channel)
{
    uint32_t value = DMA1_INTFR & flags_for(channel);
    uint32_t flags = 0u;

    if ((value & DMA_FLAG(channel, 0u)) != 0u) flags |= HAL_DMA_FLAG_GLOBAL;
    if ((value & DMA_FLAG(channel, 1u)) != 0u) flags |= HAL_DMA_FLAG_COMPLETE;
    if ((value & DMA_FLAG(channel, 2u)) != 0u) flags |= HAL_DMA_FLAG_HALF;
    if ((value & DMA_FLAG(channel, 3u)) != 0u) flags |= HAL_DMA_FLAG_ERROR;
    return flags;
}

static void clear_hw_flags(hal_dma_channel_t channel, uint32_t flags)
{
    uint32_t value = 0u;
    if ((flags & HAL_DMA_FLAG_GLOBAL) != 0u) value |= DMA_FLAG(channel, 0u);
    if ((flags & HAL_DMA_FLAG_COMPLETE) != 0u) value |= DMA_FLAG(channel, 1u);
    if ((flags & HAL_DMA_FLAG_HALF) != 0u) value |= DMA_FLAG(channel, 2u);
    if ((flags & HAL_DMA_FLAG_ERROR) != 0u) value |= DMA_FLAG(channel, 3u);
    DMA1_INTFCR = value;
}

static void dma_dispatch(void *arg)
{
    hal_dma_channel_t channel = (hal_dma_channel_t)(uintptr_t)arg;
    uint32_t flags;

    if (!valid_channel(channel) || states[channel].initialized == 0u)
        return;
    flags = flag_value(channel);
    if (flags == 0u)
        return;
    clear_hw_flags(channel, flags);
    if (states[channel].callback != NULL)
        states[channel].callback(channel, flags, states[channel].arg);
}

int hal_dma_get_instance_count(void)
{
    return 7;
}

ecos_err_t hal_dma_init(hal_dma_channel_t channel,
                        const hal_dma_config_t *config)
{
    uint32_t control = 0u;
    uint32_t state;

    if (!valid_channel(channel) || config == NULL || config->count == 0u ||
        config->direction > HAL_DMA_DIRECTION_MEMORY_TO_MEMORY ||
        !valid_width(config->peripheral_width) ||
        !valid_width(config->memory_width) || config->mode > HAL_DMA_MODE_CIRCULAR ||
        config->priority > HAL_DMA_PRIORITY_VERY_HIGH ||
        (config->direction == HAL_DMA_DIRECTION_MEMORY_TO_MEMORY &&
         config->mode == HAL_DMA_MODE_CIRCULAR))
        return ECOS_ERR_INVALID_ARGUMENT;
    if (config->peripheral_address == 0u || config->memory_address == 0u)
        return ECOS_ERR_INVALID_ARGUMENT;

    state = ch32v103_irq_lock();
    if (states[channel].initialized != 0u ||
        (DMA_CFGR(channel) & DMA_CFGR_EN) != 0u) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_BUSY;
    }
    RCC_AHBPCENR |= 1u; /* DMA1EN */
    DMA_CFGR(channel) = 0u;
    DMA_CNTR(channel) = config->count;
    DMA_PADDR(channel) = (uint32_t)config->peripheral_address;
    DMA_MADDR(channel) = (uint32_t)config->memory_address;
    if (config->direction == HAL_DMA_DIRECTION_MEMORY_TO_PERIPHERAL)
        control |= DMA_CFGR_DIR;
    else if (config->direction == HAL_DMA_DIRECTION_MEMORY_TO_MEMORY)
        control |= DMA_CFGR_MEM2MEM;
    if (config->mode == HAL_DMA_MODE_CIRCULAR) control |= DMA_CFGR_CIRC;
    if (config->peripheral_increment != 0u) control |= DMA_CFGR_PINC;
    if (config->memory_increment != 0u) control |= DMA_CFGR_MINC;
    control |= width_bits(config->peripheral_width, 8u);
    control |= width_bits(config->memory_width, 10u);
    control |= (uint32_t)config->priority << 12;
    if (config->interrupt_complete != 0u) control |= DMA_CFGR_TCIE;
    if (config->interrupt_half != 0u) control |= DMA_CFGR_HTIE;
    if (config->interrupt_error != 0u) control |= DMA_CFGR_TEIE;
    DMA_CFGR(channel) = control;
    clear_hw_flags(channel, HAL_DMA_FLAG_GLOBAL | HAL_DMA_FLAG_COMPLETE |
                   HAL_DMA_FLAG_HALF | HAL_DMA_FLAG_ERROR);
    states[channel].initialized = 1u;
    states[channel].callback_enabled = 0u;
    states[channel].interrupt_mask = control &
        (DMA_CFGR_TCIE | DMA_CFGR_HTIE | DMA_CFGR_TEIE);
    states[channel].callback = NULL;
    states[channel].arg = NULL;
    ch32v103_irq_restore(state);
    return ECOS_OK;
}

ecos_err_t hal_dma_deinit(hal_dma_channel_t channel)
{
    uint32_t state;
    ecos_err_t result = ECOS_OK;

    if (!valid_channel(channel)) return ECOS_ERR_INVALID_ARGUMENT;
    state = ch32v103_irq_lock();
    if (states[channel].initialized == 0u) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_NOT_INITIALIZED;
    }
    DMA_CFGR(channel) &= ~DMA_CFGR_EN;
    if (states[channel].callback_enabled != 0u) {
        (void)hal_interrupt_disable(DMA_IRQ(channel));
        result = hal_interrupt_free(DMA_IRQ(channel));
    }
    clear_hw_flags(channel, HAL_DMA_FLAG_GLOBAL | HAL_DMA_FLAG_COMPLETE |
                   HAL_DMA_FLAG_HALF | HAL_DMA_FLAG_ERROR);
    DMA_CNTR(channel) = 0u;
    DMA_CFGR(channel) = 0u;
    states[channel].initialized = 0u;
    states[channel].callback_enabled = 0u;
    states[channel].interrupt_mask = 0u;
    states[channel].callback = NULL;
    states[channel].arg = NULL;
    ch32v103_irq_restore(state);
    return result;
}

ecos_err_t hal_dma_start(hal_dma_channel_t channel)
{
    if (!valid_channel(channel)) return ECOS_ERR_INVALID_ARGUMENT;
    if (states[channel].initialized == 0u) return ECOS_ERR_NOT_INITIALIZED;
    DMA_CFGR(channel) |= DMA_CFGR_EN;
    return ECOS_OK;
}

ecos_err_t hal_dma_stop(hal_dma_channel_t channel)
{
    if (!valid_channel(channel)) return ECOS_ERR_INVALID_ARGUMENT;
    if (states[channel].initialized == 0u) return ECOS_ERR_NOT_INITIALIZED;
    DMA_CFGR(channel) &= ~DMA_CFGR_EN;
    return ECOS_OK;
}

ecos_err_t hal_dma_set_count(hal_dma_channel_t channel, uint16_t count)
{
    if (!valid_channel(channel) || count == 0u) return ECOS_ERR_INVALID_ARGUMENT;
    if (states[channel].initialized == 0u) return ECOS_ERR_NOT_INITIALIZED;
    if ((DMA_CFGR(channel) & DMA_CFGR_EN) != 0u) return ECOS_ERR_BUSY;
    DMA_CNTR(channel) = count;
    return ECOS_OK;
}

ecos_err_t hal_dma_get_count(hal_dma_channel_t channel, uint16_t *count)
{
    if (!valid_channel(channel) || count == NULL) return ECOS_ERR_INVALID_ARGUMENT;
    if (states[channel].initialized == 0u) return ECOS_ERR_NOT_INITIALIZED;
    *count = (uint16_t)DMA_CNTR(channel);
    return ECOS_OK;
}

ecos_err_t hal_dma_get_flags(hal_dma_channel_t channel, uint32_t *flags)
{
    if (!valid_channel(channel) || flags == NULL) return ECOS_ERR_INVALID_ARGUMENT;
    if (states[channel].initialized == 0u) return ECOS_ERR_NOT_INITIALIZED;
    *flags = flag_value(channel);
    return ECOS_OK;
}

ecos_err_t hal_dma_clear_flags(hal_dma_channel_t channel, uint32_t flags)
{
    if (!valid_channel(channel) ||
        (flags & ~(HAL_DMA_FLAG_GLOBAL | HAL_DMA_FLAG_COMPLETE |
                   HAL_DMA_FLAG_HALF | HAL_DMA_FLAG_ERROR)) != 0u)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (states[channel].initialized == 0u) return ECOS_ERR_NOT_INITIALIZED;
    clear_hw_flags(channel, flags);
    return ECOS_OK;
}

ecos_err_t hal_dma_register_callback(hal_dma_channel_t channel,
                                      hal_dma_callback_t callback,
                                      void *arg, uint32_t priority)
{
    uint32_t state;
    ecos_err_t result;

    if (!valid_channel(channel) || callback == NULL || priority == 0u ||
        priority > CH32V103_IRQ_PRIORITY_MAX)
        return ECOS_ERR_INVALID_ARGUMENT;
    if (states[channel].initialized == 0u) return ECOS_ERR_NOT_INITIALIZED;
    if (states[channel].interrupt_mask == 0u) return ECOS_ERR_INVALID_STATE;
    state = ch32v103_irq_lock();
    if (states[channel].callback_enabled != 0u) {
        ch32v103_irq_restore(state);
        return ECOS_ERR_BUSY;
    }
    result = hal_interrupt_alloc(DMA_IRQ(channel), priority, dma_dispatch,
                                 (void *)(uintptr_t)channel);
    if (result == ECOS_OK) {
        states[channel].callback = callback;
        states[channel].arg = arg;
        states[channel].callback_enabled = 1u;
        DMA_CFGR(channel) |= states[channel].interrupt_mask;
        result = hal_interrupt_enable(DMA_IRQ(channel));
        if (result != ECOS_OK) {
            DMA_CFGR(channel) &= ~(DMA_CFGR_TCIE | DMA_CFGR_HTIE | DMA_CFGR_TEIE);
            states[channel].callback_enabled = 0u;
            states[channel].interrupt_mask = 0u;
            states[channel].callback = NULL;
            states[channel].arg = NULL;
            (void)hal_interrupt_free(DMA_IRQ(channel));
        }
    }
    ch32v103_irq_restore(state);
    return result;
}
