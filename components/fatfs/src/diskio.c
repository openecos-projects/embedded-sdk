/*-----------------------------------------------------------------------*/
/* FatFs disk adapter for the SFUD managed serial flash.                 */
/*-----------------------------------------------------------------------*/

#include <stdint.h>

#include "ff.h"
#include "diskio.h"
#include "ecos/log.h"
#include "sfud.h"

#define DEV_FLASH 0u
#define FATFS_SECTOR_SIZE ((uint32_t)FF_MIN_SS)

static sfud_flash *flash;
static uint32_t sector_count;
static uint32_t erase_block_sectors;
static bool disk_ready;

static bool disk_range_valid(LBA_t sector, UINT count)
{
    uint32_t start = (uint32_t)sector;
    uint32_t count32 = (uint32_t)count;

    if (!disk_ready || flash == NULL || count32 == 0u) {
        return false;
    }
    if (start >= sector_count || count32 > sector_count - start) {
        return false;
    }
    if ((size_t)count32 > SIZE_MAX / FATFS_SECTOR_SIZE) {
        return false;
    }
    return true;
}

static uint32_t sector_address(LBA_t sector)
{
    return (uint32_t)sector * FATFS_SECTOR_SIZE;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != DEV_FLASH || !disk_ready || flash == NULL || !flash->init_ok) {
        return STA_NOINIT;
    }
    return 0u;
}

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE pdrv)
{
    uint32_t erase_gran;

    if (pdrv != DEV_FLASH) {
        return STA_NOINIT;
    }

    flash = sfud_get_device(DEV_FLASH);
    if (flash == NULL || (!flash->init_ok && sfud_init() != SFUD_SUCCESS)) {
        disk_ready = false;
        return STA_NOINIT;
    }

    if (flash->chip.capacity < FATFS_SECTOR_SIZE) {
        disk_ready = false;
        return STA_NOINIT;
    }

    sector_count = flash->chip.capacity / FATFS_SECTOR_SIZE;
    erase_gran = flash->chip.erase_gran;
    erase_block_sectors = erase_gran / FATFS_SECTOR_SIZE;
    if (erase_block_sectors == 0u) {
        erase_block_sectors = 1u;
    }
    disk_ready = sector_count != 0u;

    (void)ECOS_LOGD("fatfs", "Flash %s: %lu sectors, erase block %lu sectors",
                    flash->chip.name,
                    (unsigned long)sector_count,
                    (unsigned long)erase_block_sectors);
    return disk_status(pdrv);
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    size_t size;

    if (pdrv != DEV_FLASH || buff == NULL || !disk_range_valid(sector, count)) {
        return RES_PARERR;
    }

    size = (size_t)count * FATFS_SECTOR_SIZE;
    return sfud_read(flash, sector_address(sector), size, buff) == SFUD_SUCCESS
               ? RES_OK
               : RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    size_t size;

    if (pdrv != DEV_FLASH || buff == NULL || !disk_range_valid(sector, count)) {
        return RES_PARERR;
    }

    size = (size_t)count * FATFS_SECTOR_SIZE;
    return sfud_erase_write(flash, sector_address(sector), size, buff) == SFUD_SUCCESS
               ? RES_OK
               : RES_ERROR;
}
#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != DEV_FLASH || !disk_ready) {
        return RES_PARERR;
    }

    switch (cmd) {
    case CTRL_SYNC:
        return RES_OK;
    case GET_SECTOR_COUNT:
        if (buff == NULL) {
            return RES_PARERR;
        }
        *(DWORD *)buff = (DWORD)sector_count;
        return RES_OK;
    case GET_SECTOR_SIZE:
        if (buff == NULL) {
            return RES_PARERR;
        }
        *(WORD *)buff = (WORD)FATFS_SECTOR_SIZE;
        return RES_OK;
    case GET_BLOCK_SIZE:
        if (buff == NULL) {
            return RES_PARERR;
        }
        *(DWORD *)buff = (DWORD)erase_block_sectors;
        return RES_OK;
    case CTRL_TRIM:
        /* FF_USE_TRIM is disabled; do not erase in response to this hint. */
        return RES_PARERR;
    default:
        return RES_PARERR;
    }
}
