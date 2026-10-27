#ifndef ECOS_HAL_FLASH_H
#define ECOS_HAL_FLASH_H

#include "ecos/error.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t hal_flash_id_t;
#define HAL_FLASH_ID_0 0u

#define HAL_FLASH_BASE_PHYSICAL 0x08000000u
#define HAL_FLASH_BASE_ALIAS    0x00000000u
#define HAL_FLASH_SIZE_BYTES    (64u * 1024u)
#define HAL_FLASH_PAGE_SIZE     1024u

ecos_err_t hal_flash_get_size(hal_flash_id_t id, uint32_t *size);
ecos_err_t hal_flash_read(hal_flash_id_t id, uint32_t offset,
                          void *data, size_t size);
ecos_err_t hal_flash_write(hal_flash_id_t id, uint32_t offset,
                           const void *data, size_t size);
ecos_err_t hal_flash_erase_page(hal_flash_id_t id, uint32_t offset);
ecos_err_t hal_flash_erase_all(hal_flash_id_t id);

#ifdef __cplusplus
}
#endif

#endif
