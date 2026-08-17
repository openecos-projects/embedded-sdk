#ifndef __HAL_CLINT_H__
#define __HAL_CLINT_H__

#include <stdint.h>

typedef struct {
    uint32_t low;
    uint32_t high;
} hal_clint_value_t;

/**
 * @brief 一致读取 CLINT mtime 计数值
 */
int hal_clint_get_mtime(hal_clint_value_t *value);

/**
 * @brief 写入 CLINT mtime 计数值
 */
int hal_clint_set_mtime(const hal_clint_value_t *value);

/**
 * @brief 读取 CLINT mtimecmp 比较值
 */
int hal_clint_get_mtimecmp(hal_clint_value_t *value);

/**
 * @brief 安全写入 CLINT mtimecmp 比较值
 */
int hal_clint_set_mtimecmp(const hal_clint_value_t *value);

#endif
