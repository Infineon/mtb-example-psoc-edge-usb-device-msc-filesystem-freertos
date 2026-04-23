/*******************************************************************************
 * File Name:   mtb_sdhc_diskio.h
 *
 * Description: This file serves as the public interface for mtb_sdhc_diskio.c 
 *              and includes the necessary HDSC function prototypes used by 
 *              diskio.c interface.
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
*******************************************************************************/

#ifndef _MTB_SDHC_DISKIO_H_
#define _MTB_SDHC_DISKIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ff.h"
#include "diskio.h"
#include "mtb_hal_sdhc.h"

/******************************************************************************
* Functions
******************************************************************************/
DSTATUS mtb_sdhc_disk_status(void);
DSTATUS mtb_sdhc_disk_initialize (void);
DRESULT mtb_sdhc_disk_read(BYTE *buff, LBA_t sector, UINT count);
DRESULT mtb_sdhc_disk_write(const BYTE *buff, LBA_t sector, UINT count);
DRESULT mtb_sdhc_disk_ioctl(BYTE cmd, void *buff);
void mtb_sdhc_fatfs_init(mtb_hal_sdhc_t *sdhc_obj);

#ifdef __cplusplus
}
#endif

#endif /*_MTB_SDHC_DISKIO_H_*/

/* [] END OF FILE */