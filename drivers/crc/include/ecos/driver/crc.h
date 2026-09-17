#ifndef ECOS_DRIVER_CRC_H
#define ECOS_DRIVER_CRC_H

#include "ecos/error.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ECOS_CRC_ID_0 = 0,
    ECOS_CRC_ID_COUNT
} ecos_crc_id_t;

#define ECOS_CRC_DEFAULT ((ecos_crc_id_t)ECOS_CRC_ID_0)

/* mode 的取值含义（多项式/位宽）由 SoC 定义，见对应数据手册。 */
typedef struct {
    uint32_t init_value;
    uint32_t xor_out;
    uint8_t mode;
} ecos_crc_config_t;

#define ECOS_CRC_CONFIG_DEFAULT \
    { 0xFFFFu, 0u, 2u }

ecos_err_t ecos_crc_init(ecos_crc_id_t id, const ecos_crc_config_t *config);
ecos_err_t ecos_crc_deinit(ecos_crc_id_t id);

/* 逐字喂入待计算数据；每次 init 后开始新一轮计算。 */
ecos_err_t ecos_crc_feed(ecos_crc_id_t id, uint32_t word);
ecos_err_t ecos_crc_read(ecos_crc_id_t id, uint32_t *result);

/* 便捷接口：依次喂入 count 个字后读出结果。 */
ecos_err_t ecos_crc_compute(ecos_crc_id_t id, const uint32_t *words,
                            size_t count, uint32_t *result);

#ifdef __cplusplus
}
#endif

#endif
