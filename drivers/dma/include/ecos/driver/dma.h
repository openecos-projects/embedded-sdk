#ifndef ECOS_DRIVER_DMA_H
#define ECOS_DRIVER_DMA_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ecos_dma_channel_t;
#define ECOS_DMA_CHANNEL_INVALID ((ecos_dma_channel_t)0xFFu)

typedef enum {
    ECOS_DMA_DIRECTION_PERIPHERAL_TO_MEMORY = 0,
    ECOS_DMA_DIRECTION_MEMORY_TO_PERIPHERAL,
    ECOS_DMA_DIRECTION_MEMORY_TO_MEMORY
} ecos_dma_direction_t;

typedef enum {
    ECOS_DMA_WIDTH_BYTE = 0,
    ECOS_DMA_WIDTH_HALFWORD,
    ECOS_DMA_WIDTH_WORD
} ecos_dma_width_t;

typedef enum {
    ECOS_DMA_MODE_NORMAL = 0,
    ECOS_DMA_MODE_CIRCULAR
} ecos_dma_mode_t;

typedef enum {
    ECOS_DMA_PRIORITY_LOW = 0,
    ECOS_DMA_PRIORITY_MEDIUM,
    ECOS_DMA_PRIORITY_HIGH,
    ECOS_DMA_PRIORITY_VERY_HIGH
} ecos_dma_priority_t;

typedef struct {
    uintptr_t peripheral_address;
    uintptr_t memory_address;
    uint16_t count;
    ecos_dma_direction_t direction;
    ecos_dma_width_t peripheral_width;
    ecos_dma_width_t memory_width;
    ecos_dma_mode_t mode;
    ecos_dma_priority_t priority;
    uint8_t peripheral_increment;
    uint8_t memory_increment;
    uint8_t interrupt_complete;
    uint8_t interrupt_half;
    uint8_t interrupt_error;
} ecos_dma_config_t;

typedef void (*ecos_dma_callback_t)(ecos_dma_channel_t channel,
                                    uint32_t flags, void *arg);

#define ECOS_DMA_FLAG_GLOBAL   (1u << 0)
#define ECOS_DMA_FLAG_COMPLETE (1u << 1)
#define ECOS_DMA_FLAG_HALF     (1u << 2)
#define ECOS_DMA_FLAG_ERROR    (1u << 3)
#define ECOS_DMA_CONFIG_DEFAULT \
    { 0u, 0u, 0u, ECOS_DMA_DIRECTION_PERIPHERAL_TO_MEMORY, \
      ECOS_DMA_WIDTH_BYTE, ECOS_DMA_WIDTH_BYTE, ECOS_DMA_MODE_NORMAL, \
      ECOS_DMA_PRIORITY_LOW, 0u, 1u, 0u, 0u, 0u }

int ecos_dma_get_instance_count(void);
ecos_err_t ecos_dma_init(ecos_dma_channel_t channel,
                         const ecos_dma_config_t *config);
ecos_err_t ecos_dma_deinit(ecos_dma_channel_t channel);
ecos_err_t ecos_dma_start(ecos_dma_channel_t channel);
ecos_err_t ecos_dma_stop(ecos_dma_channel_t channel);
ecos_err_t ecos_dma_set_count(ecos_dma_channel_t channel, uint16_t count);
ecos_err_t ecos_dma_get_count(ecos_dma_channel_t channel, uint16_t *count);
ecos_err_t ecos_dma_get_flags(ecos_dma_channel_t channel, uint32_t *flags);
ecos_err_t ecos_dma_clear_flags(ecos_dma_channel_t channel, uint32_t flags);
ecos_err_t ecos_dma_register_callback(ecos_dma_channel_t channel,
                                       ecos_dma_callback_t callback,
                                       void *arg, uint32_t priority);

#ifdef __cplusplus
}
#endif

#endif
