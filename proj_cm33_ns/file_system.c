/*****************************************************************************
* File Name   : file_system.c
*
* Description : This file provides the source code to implement the operations
*                performed on the file system.
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

/*******************************************************************************
* Header Files
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_system.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/
#define MOUNT_DRIVE                (1U)

/***********************************************************************
* Global variables
*****************************************************************************/
BYTE work[FF_MAX_SS];
FATFS SDFatFs;

/*****************************************************************************
* Function Name: FRESULT_str
******************************************************************************
* Summary:
*  This function converts an FRESULT code into a human-readable string.
*    1. Maps each FRESULT code to its corresponding string representation.
*    2. Returns the string representation of the provided FRESULT code.
*
* Parameters:
*  FRESULT fr : FRESULT code to be converted into a string.
*
* Return:
*  const char* : Human-readable string corresponding to the FRESULT code.
*
*****************************************************************************/
const char* FRESULT_str(FRESULT fr)
{
    const char* str = "UNKNOWN FRESULT";
    
    switch (fr) 
    {
        case FR_OK:                str = "FR_OK"; break;
        case FR_DISK_ERR:          str = "FR_DISK_ERR"; break;
        case FR_INT_ERR:           str = "FR_INT_ERR"; break;
        case FR_NOT_READY:         str = "FR_NOT_READY"; break;
        case FR_NO_FILE:           str = "FR_NO_FILE"; break;
        case FR_NO_PATH:           str = "FR_NO_PATH"; break;
        case FR_INVALID_NAME:      str = "FR_INVALID_NAME"; break;
        case FR_EXIST:             str = "FR_EXIST"; break;
        case FR_INVALID_OBJECT:    str = "FR_INVALID_OBJECT"; break;
        case FR_NOT_ENOUGH_CORE:   str = "FR_NOT_ENOUGH_CORE"; break;
        case FR_LOCKED:            str = "FR_LOCKED"; break;
        case FR_DENIED:            str = "FR_DENIED"; break;
        case FR_MKFS_ABORTED:      str = "FR_MKFS_ABORTED"; break;
        case FR_TIMEOUT:           str = "FR_TIMEOUT"; break;
        case FR_INVALID_PARAMETER: str = "FR_INVALID_PARAMETER"; break;
        default:                   str = "UNKNOWN FRESULT"; break;
    }
    
    return str;
}

/*****************************************************************************
* Function Name: format_and_mount
******************************************************************************
* Summary:
*  This function formats the file system and mounts it.
*    1. Sets up the file system parameters for formatting.
*    2. Formats the file system using the provided work area and size.
*    3. Mounts the file system.
*    4. Sets the volume label for the file system.
*    5. Prints status messages and error messages if any operation fails.
*
* Parameters:
*  BYTE* work_area : Pointer to the working area used for formatting.
*  size_t work_size: Size of the working area.
*
* Return:
*  FRESULT : Result of the formatting and mounting operations 
             (FR_OK if successful).
*
*****************************************************************************/
static FRESULT format_and_mount(BYTE* work_area, size_t work_size)
{
    FRESULT result;
    
    const MKFS_PARM fs_param =
    {
        .fmt = FM_FAT32,
        .n_fat = 1,
        .align = 0,
        .n_root = 0,
        .au_size = 0
    };
    
    printf("Formatting file system...\n");

    do
    {
        result = f_mkfs("", &fs_param, work_area, work_size);

        if (FR_OK != result)
        {
            printf("Error formatting: %s\n", FRESULT_str(result));
            break;
        }

        result = f_mount(&SDFatFs, "", MOUNT_DRIVE);

        if (FR_OK != result)
        {
            printf("Error mounting after format: %s\n", FRESULT_str(result));
            break;
        }

        result = f_setlabel(DRIVE_LABEL_NAME);

        if (FR_OK != result)
        {
            printf("Error setting label: %s\n", FRESULT_str(result));
            break;
        }

        printf("Formatting and mounting complete.\n");
        
    } while(false);

    return result;
}

/*****************************************************************************
* Function Name: fs_initialize
******************************************************************************
* Summary:
*  This function initializes the file system. It can force a format if
*  specified.
*    1. If force_format is true, it formats and mounts the file system.
*    2. If force_format is false, it tries to mount the file system.
*    3. Handles different mount errors and formats if no file system is found.
*
* Parameters:
*  bool force_format : Flag to indicate if the file system should be forcibly 
*                      formatted.
*
* Return:
*  void
*
*****************************************************************************/
void fs_initialize(bool force_format)
{
    FRESULT result;

    if (force_format)
    {
        result = format_and_mount(work, sizeof(work));

        if (FR_OK != result)
        {
            printf("Error mounting after format: %s\n", FRESULT_str(result));
        }
    }
    
    result = f_mount(&SDFatFs, "", MOUNT_DRIVE);

    switch (result)
    {
        case FR_NO_FILESYSTEM:

            printf("No file system found.\n");
            result = format_and_mount(work, sizeof(work));
            
            if (result != FR_OK)
            {
                printf("Error mounting after format: %s\n", FRESULT_str(result));
            }          
            break;

        case FR_NOT_READY:
            printf("SD Card not present! Please insert one.\n");
            break;

        case FR_OK:
            printf("File system mounted successfully.\n");
            break;

        default:
            printf("Error mounting: %s\n", FRESULT_str(result));
            break;
    }
}

/*****************************************************************************
* Function Name: fs_create_file
******************************************************************************
* Summary:
*  This function creates a file with the specified name and writes the
*  provided content to it if it is a newly created file.
*    1. Opens the file with the given name. If the file does not exist,
*       it attempts to create a new file.
*    2. Writes the initial content to the file if it was newly created.
*    3. Handles errors during opening, writing, and closing the file.
*
* Parameters:
*  const TCHAR* fname  : Pointer to the null-terminated string that specifies
*                        the name of the file to be created.
*  const TCHAR* content: Pointer to the null-terminated string that contains
*                        the content to write to the file.
*
* Return:
*  void
*
*****************************************************************************/
void fs_create_file(const TCHAR* fname, const TCHAR* content)
{
    FIL fp;
    FRESULT result;
    UINT bytes_written;

    result = f_open(&fp, fname, FA_WRITE | FA_READ | FA_OPEN_EXISTING);

    if (FR_OK != result)
    {
        (void) f_close(&fp);

        printf("%s not found: %s \r\n", fname, FRESULT_str(result));
        printf("Creating %s file...\r\n", fname);
        result = f_open(&fp, fname, FA_WRITE | FA_READ | FA_CREATE_NEW);
        
        if (FR_OK != result)
        {
            printf("Error in creating the file %s : %s \r\n", fname, FRESULT_str(result));
        }
    }

    if (FR_OK == result)
    {
        /* Check if the file was newly created.  If so, write the initial content. */
        if (0U == f_size(&fp))
        {
            size_t content_len = strlen(content);

            result = f_write(&fp, content, content_len, &bytes_written);

            if (FR_OK != result)
            {
                printf("Error writing to %s: %s \r\n", fname, FRESULT_str(result));
            }
            else if (bytes_written != content_len)
            {
                printf("Warning: Wrote %u bytes, expected %zu. \r\n", bytes_written, content_len);
            }
            else
            {
                printf("Initial content written successfully. \r\n");
            }
        }
        else
        {
            printf("%s already exists.\r\n", fname);
        }
    }

    if (FR_OK != f_close(&fp))
    {
        printf("Error closing %s: %s \r\n", fname, FRESULT_str(result));
    }
}


/*****************************************************************************
* Function Name: fs_print_file
******************************************************************************
* Summary:
*  This function opens a file with the specified name and prints its contents
*  to the console.
*    1. Opens the file in read mode.
*    2. Reads the file content in chunks and prints to the console.
*    3. Handles errors during opening, reading, and closing the file.
*
* Parameters:
*  const TCHAR* fname : Pointer to the null-terminated string that specifies
*                       the name of the file to be read and printed.
*
* Return:
*  void
*
*****************************************************************************/
void fs_print_file(const TCHAR* fname)
{
    FIL fp;
    FRESULT result;
    UINT bytes_read;
    char buffer[FF_MAX_SS];

    printf("Opening %s for reading...\r\n", fname);

    result = f_open(&fp, fname, FA_READ);

    if (FR_OK == result)
    {
        printf("Reading from %s: \r\n", fname);

        while (true)
        {
            result = f_read(&fp, buffer, sizeof(buffer), &bytes_read);

            if (FR_OK != result)
            {
                printf("Error reading %s: %s\n", fname, FRESULT_str(result));
                
                /* Exit the loop on error */
                break;
            }

            if (bytes_read == 0)
            {
                /* End of file reached */
                break;
            }

            /* Important: Print only the bytes actually read */
            for (UINT i = 0; i < bytes_read; i++)
            {
                printf("%c", buffer[i]);
            }
        }

        printf("\r\n");
    }

    else
    {
        printf("Error opening %s for reading: %s \r\n", fname, FRESULT_str(result));
    }

    if (FR_OK != f_close(&fp))
    {
        printf("Error closing %s: %s \r\n", fname, FRESULT_str(result));
    }
}


/* [] END OF FILE */