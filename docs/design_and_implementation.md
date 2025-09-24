[Click here](../README.md) to view the README.

## Design and implementation

The design of this application is minimalistic to get started with code examples on PSOC&trade; Edge MCU devices. All PSOC&trade; Edge E84 MCU applications have a dual-CPU three-project structure to develop code for the CM33 and CM55 cores. The CM33 core has two separate projects for the secure processing environment (SPE) and non-secure processing environment (NSPE). A project folder consists of various subfolders, each denoting a specific aspect of the project. The three project folders are as follows:

**Table 1. Application projects**

Project | Description
--------|------------------------
*proj_cm33_s* | Project for CM33 secure processing environment (SPE)
*proj_cm33_ns* | Project for CM33 non-secure processing environment (NSPE)
*proj_cm55* | CM55 project

<br>

In this code example, at device reset, the secure boot process starts from the ROM boot with the secure enclave (SE) as the root of trust (RoT). From the secure enclave, the boot flow is passed on to the system CPU subsystem where the secure CM33 application starts. After all necessary secure configurations, the flow is passed on to the non-secure CM33 application. Resource initialization for this example is performed by this CM33 non-secure project. It configures the system clocks, pins, clock to peripheral connections, and other platform resources. It then enables the CM55 core using the `Cy_SysEnableCM55()` function and the CM55 core is subsequently put to DeepSleep mode.


This code example uses the FreeRTOS on the CM33 CPU. The following tasks are created in the *main.c* file:

- **USB task:** Handles the USB communication
- **Button task:** Handles the read/write communication on SD card

The firmware also uses a mutex (`rtos_fs_mutex`) to control accesses to the file system by these two tasks. FatFs is the chosen file system library to enable manipulating files in this code example. The FatFs library files are located in the *components/COMPONENT_FATFS* folder. Low-level disk I/O drivers are implemented in the *proj_cm33_ns* > *diskio.c* file. The MCU uses the SD host interface to communicate with the microSD card. The *proj_cm33_ns* > *sd_card.c* file implements a wrapper to the SD host driver.

In the *USB task*, the USB device block is configured to use the MSC device class. The task constantly checks if any USB requests are received and processes the same. It bridges the USB with the file system, allowing the PC to view all files in the microSD card.

The emUSB-device middleware requires an MSD storage driver to perform initialization, read, and write operations on the attached storage device (microSD card). With this code example, a FatFs-based storage driver is supplied in the *proj_cm33_ns* > *usb_msd_storage_fatfs.c* file.

Initializing the USB hardware block and adding the MSD device to the USB stack is implemented in the *proj_cm33_ns* > *usb_comm.c* file. This file also contains the configuration for USB IN and OUT endpoints.

The *Button task* monitors the button state to print the data in UART terminal or to format the file system and create a new file. You can access the *readme.txt* file through the USB Mass Storage device displaying in the PC when the kit is plugged to PC via its device USB connector, which allows you to modify the content of the file on the PC using a text editor. You can then display the content in the UART terminal by pressing the **USER BTN1**.