#ifndef ECOS_HAL_DMA_H
#define ECOS_HAL_DMA_H

#include "ecos/error.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t hal_dma_channel_t;

#define HAL_DMA_CHANNEL_INVALID ((hal_dma_channel_t)0xFFu)

typedef enum {
    HAL_DMA_DIRECTION_PERIPHERAL_TO_MEMORY = 0,
    HAL_DMA_DIRECTION_MEMORY_TO_PERIPHERAL,
    HAL_DMA_DIRECTION_MEMORY_TO_MEMORY
} hal_dma_direction_t;

typedef enum {
    HAL_DMA_WIDTH_BYTE = 0,
    HAL_DMA_WIDTH_HALFWORD,
    HAL_DMA_WIDTH_WORD
} hal_dma_width_t;

typedef enum {
    HAL_DMA_MODE_NORMAL = 0,
    HAL_DMA_MODE_CIRCULAR
} hal_dma_mode_t;

typedef enum {
    HAL_DMA_PRIORITY_LOW = 0,
    HAL_DMA_PRIORITY_MEDIUM,
    HAL_DMA_PRIORITY_HIGH,
    HAL_DMA_PRIORITY_VERY_HIGH
} hal_dma_priority_t;

typedef struct {
    uintptr_t peripheral_address;
    uintptr_t memory_address;
    uint16_t count;
    hal_dma_direction_t direction;
    hal_dma_width_t peripheral_width;
    hal_dma_width_t memory_width;
    hal_dma_mode_t mode;
    hal_dma_priority_t priority;
    uint8_t peripheral_increment;
    uint8_t memory_increment;
    uint8_t interrupt_complete;
    uint8_t interrupt_half;
    uint8_t interrupt_error;
} hal_dma_config_t;

typedef void (*hal_dma_callback_t)(hal_dma_channel_t channel,
                                   uint32_t flags, void *arg);

int hal_dma_get_instance_count(void);
ecos_err_t hal_dma_init(hal_dma_channel_t channel,
                        const hal_dma_config_t *config);
ecos_err_t hal_dma_deinit(hal_dma_channel_t channel);
ecos_err_t hal_dma_start(hal_dma_channel_t channel);
ecos_err_t hal_dma_stop(hal_dma_channel_t channel);
ecos_err_t hal_dma_set_count(hal_dma_channel_t channel, uint16_t count);
ecos_err_t hal_dma_get_count(hal_dma_channel_t channel, uint16_t *count);
ecos_err_t hal_dma_get_flags(hal_dma_channel_t channel, uint32_t *flags);
ecos_err_t hal_dma_clear_flags(hal_dma_channel_t channel, uint32_t flags);
ecos_err_t hal_dma_register_callback(hal_dma_channel_t channel,
                                      hal_dma_callback_t callback,
                                      void *arg, uint32_t priority);

enum {
    HAL_DMA_FLAG_GLOBAL = 1u << 0,
    HAL_DMA_FLAG_COMPLETE = 1u << 1,
    HAL_DMA_FLAG_HALF = 1u << 2,
    HAL_DMA_FLAG_ERROR = 1u << 3
};

#ifdef __cplusplus
}
#endif

#endif
