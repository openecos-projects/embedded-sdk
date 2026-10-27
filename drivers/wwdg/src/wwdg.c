#include "ecos/driver/wwdg.h"
#include "ecos/hal/wwdg.h"

ecos_err_t ecos_wwdg_init(ecos_wwdg_id_t id, const ecos_wwdg_config_t *config)
{
    return hal_wwdg_init((hal_wwdg_id_t)id, (const hal_wwdg_config_t *)config);
}

ecos_err_t ecos_wwdg_deinit(ecos_wwdg_id_t id)
{
    return hal_wwdg_deinit((hal_wwdg_id_t)id);
}

ecos_err_t ecos_wwdg_feed(ecos_wwdg_id_t id, uint8_t counter)
{
    return hal_wwdg_feed((hal_wwdg_id_t)id, counter);
}

ecos_err_t ecos_wwdg_get_status(ecos_wwdg_id_t id, uint32_t *status)
{
    return hal_wwdg_get_status((hal_wwdg_id_t)id, status);
}

ecos_err_t ecos_wwdg_clear_status(ecos_wwdg_id_t id)
{
    return hal_wwdg_clear_status((hal_wwdg_id_t)id);
}
