/*****************************************************************************
* File Name   : usb_msd_storage_fatfs.c
*
* Description : Fatfs file system interface of the mass storage device client.
*
* Note        : See README.md
*
******************************************************************************
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
*****************************************************************************/
#include "USB_Private.h"
#include "USB_MSD.h"
#include "cybsp.h"
#include "diskio.h"
#include "cy_sd_host.h"
#include "file_system.h"

/*****************************************************************************
* Function Prototypes
*****************************************************************************/
static void init_by_name(U8 Lun, const USB_MSD_INST_DATA_DRIVER *pDriverData);
static void get_info(U8 Lun, USB_MSD_INFO *pInfo);
static U32  get_read_buffer(U8 Lun, U32 SectorIndex, void **ppData, U32 NumSectors);
static I8   read_data(U8 Lun, U32 SectorIndex, void *pData, U32 NumSectors);
static U32  get_write_buffer(U8 Lun, U32 SectorIndex, void **ppData, U32 NumSectors);
static I8   write_data(U8 Lun, U32 SectorIndex, const void *pData, U32 NumSectors);
static I8   is_medium_present(U8 Lun);
static void deinit(U8 Lun);


/*****************************************************************************
* Global Variables
*****************************************************************************/
extern BYTE work[FF_MAX_SS]; /* Work area (larger is better for processing time) */

typedef struct
{
    const char   *path;
    BYTE         pDrv;
    U32          startSector;
    U32          numSectors;
    U16          sectorSize;
    void         *pSectorBuffer;
    U32          numBytes4Buffer;
} STORAGE_DRIVER_DATA;

const USB_MSD_STORAGE_API USB_MSD_StorageByName =
{
    init_by_name,
    get_info,
    get_read_buffer,
    read_data,
    get_write_buffer,
    write_data,
    is_medium_present,
    deinit
};


/*****************************************************************************
* Static data
*****************************************************************************/
static STORAGE_DRIVER_DATA driver_data[USB_MSD_MAX_UNIT];


/*****************************************************************************
* Function Name: init
******************************************************************************
* Summary:
*  Initializes the storage medium.
*
* Parameters:
*  Lun: Logical unit number. Specifies for which drive the function is called.
*  pDriverData: Pointer to a USB_MSD_INST_DATA_DRIVER structure containing
*               all information necessary for the driver initialization.
*  path: Pointer to the null-terminated string that specifies the logical drive.
*        The string without drive number means the default drive.
*
* Return:
*  None
*
******************************************************************************/
static void init(U8 Lun, const USB_MSD_INST_DATA_DRIVER * pDriverData, const char* path)
{
    FRESULT result;
    FATFS *fs = NULL;
    STORAGE_DRIVER_DATA  *pDData;
    USB_MSD_INFO   info;
    int vol;

    if (Lun >= USB_MSD_MAX_UNIT)
    {
        return;
    }

    /* Get logical drive number */
    vol = get_ldnumber(&path);
    if (vol < 0) return;

    fs = &SDFatFs;
    if (NULL != fs)
    {
        /* Attempt to mount the external memory */
        result = f_mount(fs, path, 1);

        switch (result)
        {
            case FR_NO_FILESYSTEM:
                /* No file system, create a FAT system with default parameters */
                result = f_mkfs(path, 0, work, sizeof(work));
                if (FR_OK == result)
                {
                    f_mount(fs, path, 1);
                    f_setlabel("IFX Drive");
                }
                else
                {
                    USB_PANIC("Not able to create a file system. Stopping!\r\n");
                }
                break;

            case FR_NOT_READY:
                USB_PANIC("\r\nSD Card not present! Insert one to the SD card slot\r\n");
                break;

            default:
                break;
        }
    }

    pDData = &driver_data[Lun];

    /* Initialize the STORAGE_DRIVER_DATA structure */
    pDData->pDrv          = LD2PD(vol);
    pDData->path          = path;
    pDData->startSector   = pDriverData->StartSector;
    pDData->numSectors    = pDriverData->NumSectors;

    get_info(Lun, &info);
    pDData->sectorSize    = info.SectorSize;

    if (NULL != pDriverData->pSectorBuffer)
    {
        pDData->pSectorBuffer   = pDriverData->pSectorBuffer;
        pDData->numBytes4Buffer = pDriverData->NumBytes4Buffer;
    }
}

/*****************************************************************************
* Function Name: init_by_name
******************************************************************************
* Summary:
*  Initializes the storage medium identified by logical drive number/name.
*
* Parameters:
*  Lun: Logical unit number. Specifies for which drive the function is called.
*  pDriverData: Pointer to a USB_MSD_INST_DATA_DRIVER structure containing
*               all information necessary for the driver initialization.
*
* Return:
*  None
*
*****************************************************************************/
static void init_by_name(U8 Lun, const USB_MSD_INST_DATA_DRIVER *pDriverData)
{
    const char *path;

    path = (const char *)pDriverData->pStart;

    init(Lun, pDriverData, path);
}

/*****************************************************************************
* Function Name: get_info
******************************************************************************
* Summary:
*  Retrieves storage medium information such as sector size and number of
*  sectors available.
*
* Parameters:
*  Lun: Logical unit number. Specifies for which drive the function is called.
*  pInfo: Pointer to a USB_MSD_INFO structure containing Number of available
*         sectors and Size of one sector in bytes.
*
* Return:
*  None
*
*****************************************************************************/
static void get_info(U8 Lun, USB_MSD_INFO *pInfo)
{
    STORAGE_DRIVER_DATA *pDData;

    if (Lun >= USB_MSD_MAX_UNIT)
    {
        return;
    }
    USB_MEMSET(pInfo, 0, sizeof(USB_MSD_INFO));
    pDData = &driver_data[Lun];

    if (RES_OK == disk_ioctl(pDData->pDrv, GET_SECTOR_COUNT, &pInfo->NumSectors))
    {
        /* If user assigned a num sector value,
         * ignore the value that was retrieved by the FS.
         */

        if (0U != pDData->numSectors)
        {
            pInfo->NumSectors = pDData->numSectors;
        }
        else
        {
            pDData->numSectors = pInfo->NumSectors;
        }
    }

    if (RES_OK == disk_ioctl(pDData->pDrv, GET_SECTOR_SIZE, &pInfo->SectorSize))
    {
        if (0U == pDData->sectorSize)
        {
            pDData->sectorSize = pInfo->SectorSize;
        }
    }
}

/*****************************************************************************
* Function Name: get_buffer
******************************************************************************
* Summary:
*  Returns buffer to be used for read / write operations.
*
* Parameters:
*  Lun: Logical unit number. Specifies for which drive the function is called.
*  SectorIndex: Specifies the start sector for the read / write operation.
*  ppData: Pointer to a pointer to store the read / write buffer address of the
*          driver.
*  NumSectors: Number of sectors to read / write.
*
* Return:
*  U32: Maximum number of consecutive sectors that can be read / write from /
*       into the buffer.
*
*****************************************************************************/
static U32 get_buffer(U8 Lun, U32 SectorIndex, void **ppData, U32 NumSectors)
{
    STORAGE_DRIVER_DATA   *pDData;
    U32                    numSectorsRW;

    USB_USE_PARA(SectorIndex);

    pDData  = &driver_data[Lun];
    *ppData = pDData->pSectorBuffer;

    numSectorsRW = pDData->numBytes4Buffer / pDData->sectorSize;
    numSectorsRW = SEGGER_MIN(numSectorsRW, NumSectors);
    return numSectorsRW;
}

/*****************************************************************************
* Function Name: get_read_buffer
******************************************************************************
* Summary:
*  Prepares the read function and returns a pointer to a buffer that is used
*  by the storage driver.
*
* Parameters:
*  Lun: Logical unit number. Specifies for which drive the function is called.
*  SectorIndex: Specifies the start sector for the read operation.
*  ppData: Pointer to a pointer to store the read buffer address of the
*          driver.
*  NumSectors: Number of sectors to read.
*
* Return:
*  U32: Maximum number of consecutive sectors that can be read at
*       once by the driver.
*
*****************************************************************************/
static U32 get_read_buffer(U8 Lun, U32 SectorIndex, void **ppData, U32 NumSectors)
{
    /* Same buffer is used for read & write */
    return get_buffer(Lun, SectorIndex, ppData, NumSectors);
}

/*****************************************************************************
* Function Name: read_data
******************************************************************************
* Summary:
*  Reads one or multiple consecutive sectors from the storage medium.
*
* Parameters:
*  Lun: Logical unit number. Specifies for which drive the function is called.
*  SectorIndex: Specifies the start sector from where the read operation is
*               started.
*  pData: Pointer to buffer to store the read data.
*  NumSectors: Number of sectors to read.
*
* Return:
*  status: 0 for success else failure.
*
*****************************************************************************/
static I8 read_data(U8 Lun, U32 SectorIndex, void *pData, U32 NumSectors)
{
    I8 status;

    SectorIndex += driver_data[Lun].startSector;

    status = disk_read(driver_data[Lun].pDrv, (BYTE *)pData, SectorIndex, NumSectors);

    return status;
}

/*****************************************************************************
* Function Name: get_write_buffer
******************************************************************************
* Summary:
*  Prepares the write function and returns a pointer to a buffer that is used
*  by the storage driver.
*
* Parameters:
*  Lun: Logical unit number. Specifies for which drive the function is called.
*  SectorIndex: Specifies the start sector for the write operation.
*  ppData: Pointer to a pointer to store the write buffer address of the
*          driver.
*  NumSectors: Number of sectors to write.
*
* Return:
*  U32: Maximum number of consecutive sectors that can be written into the
*       buffer.
*
*****************************************************************************/
static U32 get_write_buffer(U8 Lun, U32 SectorIndex, void **ppData, U32 NumSectors)
{
    /* Same buffer is used for read & write */
    return get_buffer(Lun, SectorIndex, ppData, NumSectors);
}

/*****************************************************************************
* Function Name: write_data
******************************************************************************
* Summary:
*  Writes one or more consecutive sectors to the storage medium.
*
* Parameters:
*  Lun: Logical unit number. Specifies for which drive the function is called.
*  SectorIndex: Specifies the start sector for the write operation.
*  pData: Pointer to data to be written to the storage medium.
*  NumSectors: Number of sectors to write.
*
* Return:
*  status: 0 for success else failure.
*
*****************************************************************************/
static I8 write_data(U8 Lun, U32 SectorIndex, const void *pData, U32 NumSectors)
{
    I8 status;

    SectorIndex += driver_data[Lun].startSector;

    status = disk_write(driver_data[Lun].pDrv, (BYTE *)pData, SectorIndex, NumSectors);

    return status;
}

/*****************************************************************************
* Function Name: is_medium_present
******************************************************************************
* Summary:
*  Checks if medium is present.
*
* Parameters:
*  Lun: Logical unit number. Specifies for which drive the function is called.
*
* Return:
*  status: 1 for Medium present else not present.
*
*****************************************************************************/
static I8 is_medium_present(U8 Lun)
{
    I8 status = 0;

    if (Cy_SD_Host_IsCardConnected(CYBSP_SDHC_1_HW))
    {
        status = 1;
    }
    else
    {
        deinit(Lun);
        status = 0;
    }

    return status;
}

/*****************************************************************************
* Function Name: deinit
******************************************************************************
* Summary:
*  De-initializes the storage medium.
*
* Parameters:
*  Lun: Logical unit number. Specifies for which drive the function is called.
*
* Return:
*  None
*
*****************************************************************************/
static void deinit(U8 Lun)
{
    if (NULL != driver_data[Lun].path)
    {
        f_unmount(driver_data[Lun].path);
    }
}

/* [] END OF FILE */
