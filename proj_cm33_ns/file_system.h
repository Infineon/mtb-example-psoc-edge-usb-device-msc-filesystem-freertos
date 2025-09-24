/******************************************************************************
* File Name   : file_system.h
*
* Description : This file contains the function prototypes and constants used 
*               in the file_system.c.
*
* Note        : See README.md
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
******************************************************************************/
#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "ff.h"
#include "retarget_io_init.h"

/******************************************************************************
* Macros
******************************************************************************/
#define DRIVE_LABEL_NAME            "IFX Drive"
#define README_FILE_NAME            "readme.txt"
#define README_INITIAL_CONTENT      "Welcome to USB MSC Example! \r\n"

/******************************************************************************
* Externs
******************************************************************************/
extern FATFS SDFatFs;

/******************************************************************************
* Functions
******************************************************************************/
void fs_initialize(bool force_format);
void fs_create_file(const TCHAR* fname, const TCHAR* content);
void fs_print_file(const TCHAR* fname);

#if defined(__cplusplus)
}
#endif

#endif /* FILE_SYSTEM_H */

/* [] END OF FILE */
