#include "ecos/driver/backup.h"
#include "ecos/hal/backup.h"

ecos_err_t ecos_backup_read(ecos_backup_id_t id, uint8_t index, uint16_t *value)
{
    return hal_backup_read((hal_backup_id_t)id, index, value);
}
ecos_err_t ecos_backup_write(ecos_backup_id_t id, uint8_t index, uint16_t value)
{
    return hal_backup_write((hal_backup_id_t)id, index, value);
}
ecos_err_t ecos_backup_reset(ecos_backup_id_t id)
{
    return hal_backup_reset((hal_backup_id_t)id);
}
