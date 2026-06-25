/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Basic definitions of FatFs */
#include "diskio.h"		/* Declarations FatFs MAI */

/* Example: Declarations of the platform and disk functions in the project */
#include "sfud.h"

/* Example: Mapping of physical drive number for each drive */
#define DEV_FLASH	0	/* Map FTL to physical drive 0 */
static sfud_flash *flash; // 不可重入变量

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS status = STA_NOINIT;
    log_debug("[FATFS]EnterStatus, waiting...\r\n");
	switch (pdrv) {
        case DEV_FLASH:
            if (flash != NULL && flash->init_ok) {
                status = 0x00;
            } else {
                uint8_t flash_status_register = 0x00;
                sfud_read_status(flash, &flash_status_register);
                if ((flash_status_register & 0x01) == 0x00) status = 0x00;
            }
            break;
	}
    log_debug("[FATFS]ExitStatus, return:%x\r\n",status);
	return status;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS status = STA_NOINIT;
    log_debug("[FATFS]EnterInit, waiting...\r\n");

	switch (pdrv) {
        case DEV_FLASH:
            flash = sfud_get_device_table() + 0;
            if (flash->init_ok) {
                status = disk_status(DEV_FLASH);
            } else if (sfud_init() == SFUD_SUCCESS) {
                status = disk_status(DEV_FLASH);
            } else {
                status = STA_NOINIT;
            }
		    break;
	}
    log_debug("[FATFS]ExitInit, return:%x\r\n",status);
	return status;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/* 设置扇区大小为FLASH（使用WW25Q64）的最小擦除区域（4KB）                      */
/*-----------------------------------------------------------------------*/
DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT status = RES_PARERR;
    log_debug("[FATFS]EnterRead, waiting...\r\n");
    // sector += 512; // 2MB之后的第一块作为第一个扇区
    uint32_t addr = sector << 12;
    size_t size = count << 12;


	switch (pdrv) {
        case DEV_FLASH:
            if(sfud_read(flash, addr, size, buff) == SFUD_SUCCESS) status = RES_OK;
            else status = RES_ERROR;
		    break;
	}
    log_debug("[FATFS]ExitRead, return:%x\r\n",status);
	return status;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT status = RES_PARERR;
    log_debug("[FATFS]EnterWrite, waiting...\r\n");

    uint32_t addr = sector << 12;
    size_t size = count << 12;

	switch (pdrv) {
        case DEV_FLASH:
            if(sfud_erase_write(flash, addr, size, buff) == SFUD_SUCCESS) status = RES_OK;
            else status = RES_ERROR;
		    break;
	}
    log_debug("[FATFS]ExitWrite, return:%x\r\n",status);
	return status;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT status = RES_PARERR;


	switch (cmd) {
        case CTRL_SYNC:
            status = RES_OK;
		    break;

        case GET_SECTOR_COUNT:
            *(DWORD *)buff = 2048;
            status = RES_OK;
		    break;

        case GET_SECTOR_SIZE:
            *(WORD *)buff = 4096;
            status = RES_OK;
		    break;
        
        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1;
            status = RES_OK;
		    break;

        case CTRL_TRIM:
            status = RES_OK;
		    break;
	}
	return status;
}

