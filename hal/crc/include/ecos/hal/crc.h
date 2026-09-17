#ifndef ECOS_HAL_CRC_H
#define ECOS_HAL_CRC_H

#include "ecos/error.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_CRC_ID_0 = 0,
    HAL_CRC_ID_COUNT
} hal_crc_id_t;

/* CTRL 位布局（ysyx-2512 冒烟实测）：bit0 使能、bit3 装载 INIT、
 * bits[6:5] 模式选择；各模式对应的多项式/位宽由 SoC 定义。 */
typedef struct {
    uint32_t init_value;
    uint32_t xor_out;
    uint8_t mode;
} hal_crc_config_t;

ecos_err_t hal_crc_init(hal_crc_id_t id, const hal_crc_config_t *config);
ecos_err_t hal_crc_deinit(hal_crc_id_t id);
ecos_err_t hal_crc_feed(hal_crc_id_t id, uint32_t word);
ecos_err_t hal_crc_read(hal_crc_id_t id, uint32_t *result);

#ifdef __cplusplus
}
#endif

#endif
