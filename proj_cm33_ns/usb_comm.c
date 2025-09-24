/*****************************************************************************
* File Name   : usb_comm.c
*
* Description : This file provides the source code to implement the USB Mass
*               Storage class requests.
*
* Note        : See README.md
*
******************************************************************************
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
*****************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include <stdio.h>
#include <stdint.h>

/* emUSB-Host header file includes */
#include "USB.h"
#include "USB_MSD.h"
/*****************************************************************************
* Macros
*****************************************************************************/
#define BUFFER_SIZE         (8192U)    /* In bytes */
#define USB_INT_INTERVAL    (64U)
#define MSD_VENDOR_ID       (0x058B)
#define MSD_PRODUCT_ID      (0x028C)

/* Used as sector buffer in order to do read/write sector bursts (~8 sectors at once) */
static uint32_t sector_buffer[BUFFER_SIZE / 4];

/*****************************************************************************
* Static data
*****************************************************************************/
const USB_DEVICE_INFO usb_deviceInfo =
{
    .VendorId      = MSD_VENDOR_ID,
    .ProductId     = MSD_PRODUCT_ID,
    .sVendorName   = "Infineon Technologies",
    .sProductName  = "USB Mass Storage",
    .sSerialNumber = "000013245678"
};

/*****************************************************************************
* String information used when inquiring the volume 0.
*****************************************************************************/
const USB_MSD_LUN_INFO usb_lun0Info =
{
    .pVendorName  = "Infineon",
    .pProductName = "MSD Volume",
    .pProductVer  = "1.00",
    .pSerialNo    = "134657890"
};


/*****************************************************************************
* Function Name: usb_add_msd
******************************************************************************
* Summary:
*  Add MSD device to the USB Stack and the logical unit.
*
* Parameters:
*  None
*
* Return:
*  None
*
*****************************************************************************/
static void usb_add_msd(void)
{

    static U8            usb_out_buffer[USB_HS_BULK_MAX_PACKET_SIZE];
    USB_MSD_INIT_DATA    InitData;
    USB_MSD_INST_DATA    InstData;
    USB_ADD_EP_INFO      EPIn;
    USB_ADD_EP_INFO      EPOut;

    memset(&InitData, 0, sizeof(InitData));
    EPIn.Flags           = 0;                             /* Flags not used */
    EPIn.InDir           = USB_DIR_IN;                    /* IN direction (Device to Host) */
    EPIn.Interval        = 0;

    EPIn.MaxPacketSize   = USB_HS_BULK_MAX_PACKET_SIZE;   /* Maximum packet size (512 for Bulk in high-speed) */

    EPIn.TransferType    = USB_TRANSFER_TYPE_BULK;        /* Endpoint type - Bulk */
    InitData.EPIn        = USBD_AddEPEx(&EPIn, NULL, 0);

    EPOut.Flags          = 0;                             /* Flags not used */
    EPOut.InDir          = USB_DIR_OUT;                   /* OUT direction (Host to Device) */
    EPOut.Interval       = 0;

    EPOut.MaxPacketSize  = USB_HS_BULK_MAX_PACKET_SIZE;   /* Maximum packet size (512 for Bulk out high-speed) */

    EPOut.TransferType   = USB_TRANSFER_TYPE_BULK;        /* Endpoint type - Bulk */
    InitData.EPOut       = USBD_AddEPEx(&EPOut, usb_out_buffer, sizeof(usb_out_buffer));

    /* Add MSD device */
    USBD_MSD_Add(&InitData);

    /* Add logical unit 0 */
    memset(&InstData, 0, sizeof(InstData));
    InstData.pAPI                       = &USB_MSD_StorageByName;
    InstData.DriverData.pStart          = (void *)"";
    InstData.DriverData.pSectorBuffer   = sector_buffer;
    InstData.DriverData.NumBytes4Buffer = sizeof(sector_buffer);
    InstData.pLunInfo                   = &usb_lun0Info;
    USBD_MSD_AddUnit(&InstData);
}



/*****************************************************************************
* Function Name: usb_comm_init
******************************************************************************
* Summary:
*  Initializes the USB hardware block and emUSB-Device MSD class.
*
* Parameters:
*  None
*
* Return:
*  None
*
*****************************************************************************/
void usb_comm_init(void)
{
    /* Initialize the USB Device stack */
    USBD_Init();

    /* Add mass storage device to USB stack */
    usb_add_msd();

    /* Set USB device information for enumeration */
    USBD_SetDeviceInfo(&usb_deviceInfo);

    /* Start the USB device stack */
    USBD_Start();
}

/* [] END OF FILE */
