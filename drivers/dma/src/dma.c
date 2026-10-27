#include "ecos/driver/dma.h"
#include "ecos/hal/dma.h"

int ecos_dma_get_instance_count(void)
{
    return hal_dma_get_instance_count();
}

ecos_err_t ecos_dma_init(ecos_dma_channel_t channel,
                         const ecos_dma_config_t *config)
{
    return hal_dma_init(channel, (const hal_dma_config_t *)config);
}

ecos_err_t ecos_dma_deinit(ecos_dma_channel_t channel)
{
    return hal_dma_deinit(channel);
}

ecos_err_t ecos_dma_start(ecos_dma_channel_t channel)
{
    return hal_dma_start(channel);
}

ecos_err_t ecos_dma_stop(ecos_dma_channel_t channel)
{
    return hal_dma_stop(channel);
}

ecos_err_t ecos_dma_set_count(ecos_dma_channel_t channel, uint16_t count)
{
    return hal_dma_set_count(channel, count);
}

ecos_err_t ecos_dma_get_count(ecos_dma_channel_t channel, uint16_t *count)
{
    return hal_dma_get_count(channel, count);
}

ecos_err_t ecos_dma_get_flags(ecos_dma_channel_t channel, uint32_t *flags)
{
    return hal_dma_get_flags(channel, flags);
}

ecos_err_t ecos_dma_clear_flags(ecos_dma_channel_t channel, uint32_t flags)
{
    return hal_dma_clear_flags(channel, flags);
}

ecos_err_t ecos_dma_register_callback(ecos_dma_channel_t channel,
                                       ecos_dma_callback_t callback,
                                       void *arg, uint32_t priority)
{
    return hal_dma_register_callback(channel, (hal_dma_callback_t)callback, arg, priority);
}
