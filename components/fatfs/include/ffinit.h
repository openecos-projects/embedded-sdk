#ifndef __MYFILESYSTEM_H__
#define __MYFILESYSTEM_H__

#include <stdbool.h>

#include "ff.h"

extern FATFS fs_Flash;
extern FRESULT res_Flash;

/* Mount the existing volume without changing its contents. */
bool load_filesystem(void);

/* Explicitly create a new volume, then mount it. This erases the volume. */
FRESULT format_filesystem(void);

#endif
