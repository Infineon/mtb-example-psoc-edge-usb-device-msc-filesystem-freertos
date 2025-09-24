/*******************************************************************************
 * File Name:   mtb_sdhc_diskio.h
 *
 * Description: This file serves as the public interface for mtb_sdhc_diskio.c 
 *              and includes the necessary HDSC function prototypes used by 
 *              diskio.c interface.
 *
 *******************************************************************************
* Copyright 2024-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#ifndef _MTB_SDHC_DISKIO_H_
#define _MTB_SDHC_DISKIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ff.h"
#include "diskio.h"
#include "mtb_hal_sdhc.h"
#include "mtb_sdhc_diskio.h"

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