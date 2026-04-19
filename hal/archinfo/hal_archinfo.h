#ifndef __HAL_ARCHINFO_H__
#define __HAL_ARCHINFO_H__

#include <stdint.h>

void hal_archinfo_info(void);
void hal_archinfo_set_sys(uint32_t val);
void hal_archinfo_set_idl(uint32_t val);
void hal_archinfo_set_idh(uint32_t val);
uint32_t hal_archinfo_get_sys(void);
uint32_t hal_archinfo_get_idl(void);
uint32_t hal_archinfo_get_idh(void);

#endif
