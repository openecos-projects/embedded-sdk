#ifndef __HAL_SDRAM_H__
#define __HAL_SDRAM_H__

/**
 * @brief 初始化板级 SDRAM 控制器和外部存储器
 *
 * @return int 0 表示成功，-1 表示初始化失败
 */
int hal_sdram_init(void);

#endif
