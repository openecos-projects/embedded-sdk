#include "ecos/driver/flash.h"
#include "ecos/hal/flash.h"

ecos_err_t ecos_flash_get_size(ecos_flash_id_t id, uint32_t *size)
{
    return hal_flash_get_size((hal_flash_id_t)id, size);
}

ecos_err_t ecos_flash_read(ecos_flash_id_t id, uint32_t offset,
                           void *data, size_t size)
{
    return hal_flash_read((hal_flash_id_t)id, offset, data, size);
}

ecos_err_t ecos_flash_write(ecos_flash_id_t id, uint32_t offset,
                            const void *data, size_t size)
{
    return hal_flash_write((hal_flash_id_t)id, offset, data, size);
}

ecos_err_t ecos_flash_erase_page(ecos_flash_id_t id, uint32_t offset)
{
    return hal_flash_erase_page((hal_flash_id_t)id, offset);
}

ecos_err_t ecos_flash_erase_all(ecos_flash_id_t id)
{
    return hal_flash_erase_all((hal_flash_id_t)id);
}
