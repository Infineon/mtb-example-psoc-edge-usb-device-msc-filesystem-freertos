/******************************************************************************
* File Name   : mtb_sdhc_diskio.c
*
* Description : This file provides the hardware-dependent SD Card disk I/O 
*               functions for FATFS, enabling the library to perform read, 
*               write, and control operations on SD Card.
*
*******************************************************************************
* (c) 2024-2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
******************************************************************************/

#include "ff.h"
#include "diskio.h"
#include "cybsp.h"
#include "cycfg.h"
#include "mtb_hal_sdhc.h"
#include "retarget_io_init.h"
#include "mtb_sdhc_diskio.h"

/* Definitions of physical drive number for each drive */
#define DEV_MMC                   (0U)
#define SDHC_GET_SECTOR_SIZE()    (CY_SD_HOST_BLOCK_SIZE)
#define SDHC_GET_BLOCK_SIZE()     (8U)
#define SDHC_GET_SECTOR_COUNT()   (sdhc_handle->sdxx.context->maxSectorNum)

/*****************************************************************************
 * Global Variables
 *****************************************************************************/
static mtb_hal_sdhc_t *sdhc_handle = NULL;
static DSTATUS mtb_sdhc_disk_stat = RES_NOTRDY;

/*****************************************************************************
* Function Name: mtb_sdhc_fatfs_init
******************************************************************************
* Summary:
* This function initializes the SDHC FAT file system handle.
* 1. Stores the provided SDHC object pointer into a global handle.
*
* Parameters:
* mtb_hal_sdhc_t *sdhc_obj : Pointer to the SDHC object.
*
* Return:
* void : This function does not return a value.
*
*****************************************************************************/

void mtb_sdhc_fatfs_init(mtb_hal_sdhc_t *sdhc_obj)
{
    sdhc_handle = sdhc_obj;
}

/*****************************************************************************
* Function Name: mtb_sdhc_disk_status
******************************************************************************
* Summary:
* This function retrieves the current status of the SDHC disk.
* 1. Checks if the global disk status indicates that the disk is not initialized.
* 2. If not initialized, returns STA_NOINIT.
* 3. Otherwise, returns the current global disk status.
*
* Parameters:
* void : This function takes no parameters.
*
* Return:
* DSTATUS : The current status of the SDHC disk.
*
*****************************************************************************/

DSTATUS mtb_sdhc_disk_status(void)
{
    DSTATUS stat = RES_ERROR;

    /* Check if the global disk status is set to STA_NOINIT */
    if (RES_OK != mtb_sdhc_disk_stat)
    {
        stat = STA_NOINIT;
    }
    else
    {
        /* Set stat to the current global disk status */
        stat = mtb_sdhc_disk_stat;
    }

    return stat;
}

/*****************************************************************************
* Function Name: mtb_sdhc_disk_initialize
******************************************************************************
* Summary:
* This function initializes the SDHC disk.
* 1. Checks if an SD card is inserted using the SDHC HAL.
* 2. If a card is inserted, sets the global disk status to RES_OK and returns 
     RES_OK.
* 3. If no card is inserted, sets the global disk status to STA_NODISK and 
     returns STA_NODISK.
*
* Parameters:
* void : This function takes no parameters.
*
* Return:
* DSTATUS : The status of the SDHC disk initialization.
*
*****************************************************************************/

DSTATUS mtb_sdhc_disk_initialize(void)
{
    DSTATUS stat = STA_NOINIT;

    /* Check if the SD card is inserted */
    if (mtb_hal_sdhc_is_card_inserted(sdhc_handle))
    {
        /* Card is inserted, update status to RES_OK */
        mtb_sdhc_disk_stat = RES_OK;
        stat = RES_OK;
    }
    else
    {
        /* Card is not inserted, update status to STA_NODISK */
        mtb_sdhc_disk_stat = STA_NODISK;
        stat = STA_NODISK;
    }

    return stat;
}

/*****************************************************************************
* Function Name: mtb_sdhc_disk_read
******************************************************************************
* Summary:
* This function reads sectors from the SDHC disk.
* 1. Initiates an asynchronous read operation using the SDHC HAL.
* 2. Waits for the read operation to complete.
* 3. Checks the result of the read operation.
* 4. Returns RES_OK if successful, RES_ERROR otherwise.
*
* Parameters:
* BYTE *buff : Pointer to the buffer to store the read data.
* LBA_t sector : Logical Block Address of the first sector to read.
* UINT count : Number of sectors to read.
*
* Return:
* DRESULT : Result of the disk read operation (RES_OK or RES_ERROR).
*
*****************************************************************************/

DRESULT mtb_sdhc_disk_read(BYTE *buff, LBA_t sector, UINT count)
{
    cy_rslt_t cy_rslt;
    DRESULT d_rslt = RES_OK;

    cy_rslt = mtb_hal_sdhc_read_async(sdhc_handle, sector, buff, 
                                        (size_t *)&count);

    mtb_hal_sdhc_wait_transfer_complete(sdhc_handle);

    if (CY_RSLT_SUCCESS != cy_rslt)
    {
        d_rslt = RES_ERROR;
    }

    return d_rslt;
}

/*****************************************************************************
* Function Name: mtb_sdhc_disk_write
******************************************************************************
* Summary:
* This function writes sectors to the SDHC disk.
* 1. Initiates an asynchronous write operation using the SDHC HAL.
* 2. Waits for the write operation to complete.
* 3. Checks the result of the write operation.
* 4. Returns RES_OK if successful, RES_ERROR otherwise.
*
* Parameters:
* const BYTE *buff : Pointer to the buffer containing the data to write.
* LBA_t sector : Logical Block Address of the first sector to write.
* UINT count : Number of sectors to write.
*
* Return:
* DRESULT : Result of the disk write operation (RES_OK or RES_ERROR).
*
*****************************************************************************/

DRESULT mtb_sdhc_disk_write(const BYTE *buff, LBA_t sector, UINT count)
{
    cy_rslt_t cy_rslt;
    DRESULT d_rslt = RES_OK;

    cy_rslt = mtb_hal_sdhc_write_async(sdhc_handle, sector, buff, 
                                            (size_t *)&count);

    mtb_hal_sdhc_wait_transfer_complete(sdhc_handle);

    if (CY_RSLT_SUCCESS != cy_rslt)
    {
        d_rslt = RES_ERROR;
    }

    return d_rslt;
}

/*****************************************************************************
* Function Name: mtb_sdhc_disk_ioctl
******************************************************************************
* Summary:
* This function performs control operations on the SDHC disk.
* 1. Handles various control commands such as synchronization, getting sector
*    count, sector size, and block size.
* 2. Populates the provided buffer with the requested information.
* 3. Returns RES_OK if the command is handled successfully, RES_PARERR otherwise.
*
* Parameters:
* BYTE cmd : Control command to perform.
* void *buff : Pointer to the buffer to store the result.
*
* Return:
* DRESULT : Result of the control operation.
*
*****************************************************************************/

DRESULT mtb_sdhc_disk_ioctl(BYTE cmd, void *buff)
{
    DRESULT d_rslt = RES_OK;

    switch(cmd)
    {
        case CTRL_SYNC:
            break;
        case GET_SECTOR_COUNT:
            *(DWORD *) buff = SDHC_GET_SECTOR_COUNT();
            break;
        case GET_SECTOR_SIZE:
            *(WORD *) buff = SDHC_GET_SECTOR_SIZE();
            break;
        case GET_BLOCK_SIZE:
            *(DWORD *) buff = SDHC_GET_BLOCK_SIZE();
            break;
        default:
            d_rslt = RES_PARERR;
            break;
    }

    return d_rslt;
}

/* [] END OF FILE */