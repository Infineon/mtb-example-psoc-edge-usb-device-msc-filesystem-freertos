/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various existing       */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"
#include "diskio.h"
#include "mtb_sdhc_diskio.h"

#define DEV_MMC        0
#define DEV_RAM        1
#define DEV_USB        2

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (BYTE pdrv)
{
    DSTATUS stat = STA_NOINIT;

    switch (pdrv) 
    {
        case DEV_MMC :
        {
            stat = mtb_sdhc_disk_status();
        }
    }

    return stat;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (BYTE pdrv)
{
    DSTATUS stat = STA_NOINIT;

    switch (pdrv) 
    {
        case DEV_MMC :
        {
            stat = mtb_sdhc_disk_initialize();
        }
    }

    return stat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    DRESULT res = RES_ERROR;

    switch (pdrv) 
    {
        case DEV_MMC :
        {
            res = mtb_sdhc_disk_read(buff, sector, count);
        }
    }

    return res;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    DRESULT res = RES_ERROR;

    switch (pdrv) 
    {
        case DEV_MMC :
        {
            res = mtb_sdhc_disk_write(buff, sector, count);
        }
    }

    return res;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    DRESULT res = RES_ERROR;

    switch (pdrv) 
    {
        case DEV_MMC :
        {
            res = mtb_sdhc_disk_ioctl(cmd, buff);
        }
    }

    return res;
}
