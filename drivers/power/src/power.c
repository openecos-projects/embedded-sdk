#include "ecos/driver/power.h"
#include "ecos/hal/power.h"

ecos_err_t ecos_power_set_backup_access(ecos_power_id_t id, uint8_t enable)
{
    return hal_power_set_backup_access((hal_power_id_t)id, enable);
}
ecos_err_t ecos_power_set_wakeup_pin(ecos_power_id_t id, uint8_t enable)
{
    return hal_power_set_wakeup_pin((hal_power_id_t)id, enable);
}
ecos_err_t ecos_power_set_pvd(ecos_power_id_t id, uint8_t enable, uint8_t level)
{
    return hal_power_set_pvd((hal_power_id_t)id, enable, level);
}
ecos_err_t ecos_power_get_flags(ecos_power_id_t id, uint32_t *flags)
{
    return hal_power_get_flags((hal_power_id_t)id, flags);
}
ecos_err_t ecos_power_clear_flags(ecos_power_id_t id, uint32_t flags)
{
    return hal_power_clear_flags((hal_power_id_t)id, flags);
}
ecos_err_t ecos_power_enter(ecos_power_id_t id, ecos_power_mode_t mode)
{
    return hal_power_enter((hal_power_id_t)id, (hal_power_mode_t)mode);
}
