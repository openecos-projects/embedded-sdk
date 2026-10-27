#ifndef ECOS_DRIVER_FLASH_H
#define ECOS_DRIVER_FLASH_H

#include "ecos/error.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ecos_flash_id_t;
#define ECOS_FLASH_DEFAULT ((ecos_flash_id_t)0u)
#define ECOS_FLASH_PAGE_SIZE 1024u

ecos_err_t ecos_flash_get_size(ecos_flash_id_t id, uint32_t *size);
ecos_err_t ecos_flash_read(ecos_flash_id_t id, uint32_t offset,
                           void *data, size_t size);
ecos_err_t ecos_flash_write(ecos_flash_id_t id, uint32_t offset,
                            const void *data, size_t size);
ecos_err_t ecos_flash_erase_page(ecos_flash_id_t id, uint32_t offset);
ecos_err_t ecos_flash_erase_all(ecos_flash_id_t id);

#ifdef __cplusplus
}
#endif

#endif
