/*******************************************************************************
* File Name        : main.c
*
* Description      : This is the source code for the USB device mass storage
*                    file system example for ModusToolbox.
*
* Related Document : See README.md
*
********************************************************************************
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

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cybsp.h"
#include "cy_pdl.h"
#include "cyabs_rtos.h"
#include "cyabs_rtos_impl.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "cy_time.h"
#include "retarget_io_init.h"
#include "file_system.h"
#include "mtb_sdhc_diskio.h"
#include "USB.h"
#include "USB_MSD.h"
#include "usb_comm.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* RTOS Configuration */ 
#define RTOS_STACK_DEPTH                ((configMINIMAL_STACK_SIZE) * 8U)
#define BUTTON_TASK_PRIORITY            ((configMAX_PRIORITIES) - 1U)
#define USB_TASK_PRIORITY               ((configMAX_PRIORITIES) - 2U)

/* Timing and Delays (milliseconds, microseconds) */
#define CM55_BOOT_WAIT_TIME_USEC        (10U)
#define USB_TASK_LOOP_DELAY_MSEC        (10U)
#define BUTTON_SCAN_DELAY_MSEC          (10U)
#define SHORT_PRESS_DELAY_MSEC          (10U)
#define LONG_PRESS_DELAY_MSEC           (3000U)

/* Interrupt Configuration */
#define GPIO_INTERRUPT_PRIORITY         (7U)
#define USER_BTN1_PORT_MASK             (0x01UL << CYBSP_USER_BTN1_PORT_NUM)

/* SD Card Configuration */ 
#define SD_CARD_PRESENT                 (1U)

/* Enabling or disabling a MCWDT requires a wait time of upto 2 CLK_LF cycles  
 * to come into effect. This wait time value will depend on the actual CLK_LF  
 * frequency set by the BSP.
 */
#define LPTIMER_0_WAIT_TIME_USEC        (62U)

/* Define the LPTimer interrupt priority number. '1' implies highest priority.*/
#define APP_LPTIMER_INTERRUPT_PRIORITY  (1U)

/* App boot address for CM55 project */
#define CM55_APP_BOOT_ADDR          (CYMEM_CM33_0_m55_nvm_START + \
                                        CYBSP_MCUBOOT_HEADER_SIZE)

/*******************************************************************************
 * Global Variables
 ******************************************************************************/
 /* LPTimer HAL object */
static mtb_hal_lptimer_t lptimer_obj;

/* Static instance of the SDHC HAL object */
static mtb_hal_sdhc_t sdhc_obj;

/* Static context structure for the SDHC host PDL Object*/
static cy_stc_sd_host_context_t sdhc_host_context;

/* RTC HAL object */
static mtb_hal_rtc_t rtc_obj;

/* Define the semaphore handle */
static SemaphoreHandle_t button_semaphore;

/* Interrupt config structure */
cy_stc_sysint_t sysint_cfg =
{
    .intrSrc = CYBSP_USER_BTN_IRQ,
    .intrPriority = GPIO_INTERRUPT_PRIORITY
};

/*******************************************************************************
* Function Name: setup_clib_support
********************************************************************************
* Summary:
*    1. This function configures and initializes the Real-Time Clock (RTC).
*    2. It then initializes the RTC HAL object to enable CLIB support library 
*       to work with the provided Real-Time Clock (RTC) module.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void setup_clib_support(void)
{
    /* RTC Initialization */
    Cy_RTC_Init(&CYBSP_RTC_config);
    Cy_RTC_SetDateAndTime(&CYBSP_RTC_config);

    /* Initialize the ModusToolbox CLIB support library */
    mtb_clib_support_init(&rtc_obj);
}

/*******************************************************************************
* Function Name: lptimer_interrupt_handler
********************************************************************************
* Summary:
* Interrupt handler function for LPTimer instance. 
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void lptimer_interrupt_handler(void)
{
    mtb_hal_lptimer_process_interrupt(&lptimer_obj);
}

/*******************************************************************************
* Function Name: gpio_interrupt_handler
********************************************************************************
* Summary:
* This function handles the GPIO interrupt triggered by a button press.
*  - Gives a semaphore to signal a button press event.
*  - Clears the GPIO interrupt and pending NVIC interrupt.
*  - Performs a context switch if a higher priority task was woken.
*
* Parameters:
* void : This function takes no parameters.
*
* Return:
* void : This function does not return a value.
*
*******************************************************************************/
static void gpio_interrupt_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (Cy_GPIO_GetInterruptStatus(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN))
    {
        xSemaphoreGiveFromISR(button_semaphore, &xHigherPriorityTaskWoken);
    }

    Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN);
    NVIC_ClearPendingIRQ(CYBSP_USER_BTN1_IRQ);

    /* CYBSP_USER_BTN1 (SW2) and CYBSP_USER_BTN2 (SW4) share the same port and
     * hence they share the same NVIC IRQ line. Since both the buttons are
     * configured for falling edge interrupt in the BSP, pressing any button
     * will trigger the execution of this ISR. Therefore, we must clear the
     * interrupt flag of the user button (CYBSP_USER_BTN2) to avoid issues in
     * case if user presses BTN2 by mistake.
     */
    Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN2_PORT, CYBSP_USER_BTN2_PIN);
    NVIC_ClearPendingIRQ(CYBSP_USER_BTN2_IRQ);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*******************************************************************************
* Function Name: button_task
********************************************************************************
* Summary:
*  This function handles the button press task in a FreeRTOS environment.
*  - Initializes the file system and creates an initial file.
*  - Handles debounce logic to avoid false triggers.
*  - Waits for the semaphore to be taken from the ISR.
*  - Performs specific actions based on the duration of the button press:
*    - Short press: Prints the contents of a file.
*    - Long press: Initializes the file system and creates a new file.
*
* Parameters:
*  void *pvParameters : Pointer to arguments (not used in this implementation).
*
* Return:
*  void
*
*******************************************************************************/

void button_task(void *pvParameters)
{
    CY_UNUSED_PARAMETER(pvParameters);
    TickType_t last_button_press_time = 0;
    TickType_t long_press_start_time = 0;
    BaseType_t long_press_detected = pdFALSE;

    /* Initialize file system and create initial file */
    fs_initialize(false);
    fs_create_file(README_FILE_NAME, README_INITIAL_CONTENT);

    for (;;)
    {
        /* Wait for the semaphore to be taken from the ISR */
        if (xSemaphoreTake(button_semaphore, portMAX_DELAY) ==
            pdTRUE)
        {
            if (Cy_GPIO_Read(CYBSP_SW2_PORT, CYBSP_SW2_PIN) ==
                CYBSP_BTN_PRESSED)
            {
                last_button_press_time = xTaskGetTickCount();
                long_press_start_time = xTaskGetTickCount();
                long_press_detected = pdFALSE;

                /* Polling for button status to detect long/short press */
                while (Cy_GPIO_Read(CYBSP_SW2_PORT, CYBSP_SW2_PIN) ==
                       CYBSP_BTN_PRESSED)
                {
                    if (!long_press_detected &&
                        ((xTaskGetTickCount() - long_press_start_time) >=
                        pdMS_TO_TICKS(LONG_PRESS_DELAY_MSEC)))
                    {
                        long_press_detected = pdTRUE;
                        printf("\r\n--- Button long-pressed! ---\r\n");
                        fs_initialize(true);
                        fs_create_file(README_FILE_NAME,
                                       README_INITIAL_CONTENT);
                    }

                    vTaskDelay(pdMS_TO_TICKS(BUTTON_SCAN_DELAY_MSEC));
                }

                /* Button released, check for short press */
                if (!long_press_detected &&
                    ((xTaskGetTickCount() - last_button_press_time) >=
                    pdMS_TO_TICKS(SHORT_PRESS_DELAY_MSEC)))
                {
                    printf("\r\n--- Button short-pressed! ---\r\n");
                    fs_print_file(README_FILE_NAME);
                }
            }
        }
    }
}

/*******************************************************************************
* Function Name: usb_task
********************************************************************************
* Summary:
*  This function handles the USB task operations in a FreeRTOS environment.
*   - Initializes and enumerates the USB communication.
*   - Continuously checks and handles USB state, performing tasks like polling 
*     the USB MSD and toggling an LED.
*
* Parameters:
*  void *arg : Pointer to arguments (not used in this implementation).
*
* Return:
*  void
*
*******************************************************************************/
static void usb_task(void *arg)
{
    /* Initialize the USB communication stack and start enumeration. */
    usb_comm_init();

    for (;;)
    {
        /* Check if the USB device is configured (and not suspended).
         * USB_STAT_CONFIGURED indicates that the host has successfully
         * configured the device. USB_STAT_SUSPENDED indicates that the
         * host has suspended the device.
         */
        if ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED))
             == USB_STAT_CONFIGURED)
        {
            /* Poll the Mass Storage Device (MSD) class for data transfers.
             * This function handles the data transfers for the MSD class,
             * such as reading and writing to the storage device.
             */
            USBD_MSD_Poll();

            /* This provides a visual indication that the USB task is running 
             * and the MSD is being polled.
             */
            Cy_GPIO_Inv(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);
        }
        else
        {
            /* The user LED is turned off to signal that the USB device is 
             * either not configured or is in a suspended state.
             */
            Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN,
                          CYBSP_LED_STATE_OFF);
        }

        /* Delay the task for a specified time. */
        vTaskDelay(pdMS_TO_TICKS(USB_TASK_LOOP_DELAY_MSEC));
    }
}

/*******************************************************************************
* Function Name: Cy_SD_Host_IsCardConnected
********************************************************************************
* Summary:
* This function checks if an SD card is connected to the SD host.
*  - Reads the state of the SD card detect GPIO pin.
*  - Returns true if the card is detected as present, false otherwise.
*
* Parameters:
* SDHC_Type const *base : Pointer to the SD host base address (unused).
*
* Return:
* bool : True if the SD card is connected, false otherwise.
*
*******************************************************************************/

bool Cy_SD_Host_IsCardConnected(SDHC_Type const *base)
{
    CY_UNUSED_PARAMETER(base);

    return ((SD_CARD_PRESENT == Cy_GPIO_Read(CYBSP_SDHC_DETECT_PORT,
                                CYBSP_SDHC_DETECT_PIN)) ? false : true);
}

/*******************************************************************************
* Function Name: setup_tickless_idle_timer
********************************************************************************
* Summary:
* 1. This function first configures and initializes an interrupt for LPTimer.
* 2. Then it initializes the LPTimer HAL object to be used in the RTOS 
*    tickless idle mode implementation to allow the device enter deep sleep 
*    when idle task runs. LPTIMER_0 instance is configured for CM33 CPU.
* 3. It then passes the LPTimer object to abstraction RTOS library that 
*    implements tickless idle mode
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void setup_tickless_idle_timer(void)
{
    /* Interrupt configuration structure for LPTimer */
    cy_stc_sysint_t lptimer_intr_cfg =
    {
        .intrSrc = CYBSP_CM33_LPTIMER_0_IRQ,
        .intrPriority = APP_LPTIMER_INTERRUPT_PRIORITY
    };

    /* Initialize the LPTimer interrupt and specify the interrupt handler. */
    cy_en_sysint_status_t interrupt_init_status = 
                                    Cy_SysInt_Init(&lptimer_intr_cfg, 
                                                    lptimer_interrupt_handler);
    
    /* LPTimer interrupt initialization failed. Stop program execution. */
    if(CY_SYSINT_SUCCESS != interrupt_init_status)
    {
        handle_app_error();
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(lptimer_intr_cfg.intrSrc);

    /* Initialize the MCWDT block */
    cy_en_mcwdt_status_t mcwdt_init_status = 
                                    Cy_MCWDT_Init(CYBSP_CM33_LPTIMER_0_HW, 
                                                &CYBSP_CM33_LPTIMER_0_config);

    /* MCWDT initialization failed. Stop program execution. */
    if(CY_MCWDT_SUCCESS != mcwdt_init_status)
    {
        handle_app_error();
    }
  
    /* Enable MCWDT instance */
    Cy_MCWDT_Enable(CYBSP_CM33_LPTIMER_0_HW,
                    CY_MCWDT_CTR_Msk, 
                    LPTIMER_0_WAIT_TIME_USEC);

    /* Setup LPTimer using the HAL object and desired configuration as defined
     * in the device configurator. */
    cy_rslt_t result = mtb_hal_lptimer_setup(&lptimer_obj, 
                                            &CYBSP_CM33_LPTIMER_0_hal_config);
    
    /* LPTimer setup failed. Stop program execution. */
    if(CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Pass the LPTimer object to abstraction RTOS library that implements 
     * tickless idle mode 
     */
    cyabs_rtos_set_lptimer(&lptimer_obj);
}

/*******************************************************************************
* Function Name: init_sdhc_fatfs
********************************************************************************
* Summary:
* This function initializes the SDHC hardware and FAT file system.
* 1. Enables the SD host and initializes it with the provided configurations.
* 2. Initializes the SD card and checks for successful connection.
* 3. Sets up the SDHC HAL (Hardware Abstraction Layer).
* 4. Initializes the SD card detect GPIO pin.
* 5. Initializes the SDHC FAT file system.
* 6. Returns the result of the HAL setup.
*
* Parameters:
* void : This function takes no parameters.
*
* Return:
* cy_rslt_t : Result of the SDHC HAL setup, indicating success or failure.
*
*******************************************************************************/
cy_rslt_t init_sdhc_fatfs(void)
{
    cy_rslt_t result = CY_SD_HOST_ERROR;
    cy_en_sd_host_status_t pdl_sdhc_status;

    /* The SD Card should be enabled before calling any other SD Card APIs */
    Cy_SD_Host_Enable(CYBSP_SDHC_1_HW);

    do
    {
        pdl_sdhc_status = Cy_SD_Host_Init(CYBSP_SDHC_1_HW, &CYBSP_SDHC_1_config,
                                          &sdhc_host_context);

        if (CY_SD_HOST_SUCCESS != pdl_sdhc_status)
        {
            printf("SD Host Initialization failed on SDHC instance with "
                   "status: 0x%X\r\n", pdl_sdhc_status);
            break;
        }

        pdl_sdhc_status = Cy_SD_Host_InitCard(CYBSP_SDHC_1_HW,
                                              &CYBSP_SDHC_1_card_cfg,
                                              &sdhc_host_context);

        if (CY_SD_HOST_SUCCESS != pdl_sdhc_status)
        {
            printf("SD Host Initialization failed: Check SD card insertion and "
                   "connection.\r\n");
            break;
        }

        result = mtb_hal_sdhc_setup(&sdhc_obj, &CYBSP_SDHC_1_sdhc_hal_config,
                                    NULL, &sdhc_host_context);

        if (CY_RSLT_SUCCESS != result)
        {
            printf("SDHC HAL setup failed with status: 0x%X\r\n", pdl_sdhc_status);
            break;
        }

        /* Initialize the SDHC FAT file system with the SDHC HAL object. */
        mtb_sdhc_fatfs_init(&sdhc_obj);

    } while (false);

    return result;
}

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* Initializes the system, including hardware, interrupts, SDHC, USB, and 
* FreeRTOS tasks. Starts the RTOS scheduler and handles potential errors.
*
* Parameters:
* void : This function takes no parameters.
*
* Return:
* int : Returns 0 if the application terminates (should not happen).
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;
    BaseType_t task_return;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board initialization failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Setup CLIB support library. */
    setup_clib_support();

    /* Enable global interrupts */
    __enable_irq();

    /* Setup the LPTimer instance for CM33 CPU. */
    setup_tickless_idle_timer();

    /* CYBSP_USER_BTN1 (SW2) and CYBSP_USER_BTN2 (SW4) share the same port and 
    * hence they share the same NVIC IRQ line. Since both are configured in the BSP
    * via the Device Configurator, the interrupt flags for both the buttons are set
    * right after they get initialized through the call to cybsp_init(). The flags
    * must be cleared otherwise the interrupt line will be constantly asserted.
    */
    Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN);
    Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN2_PORT, CYBSP_USER_BTN2_PIN);
    NVIC_ClearPendingIRQ(CYBSP_USER_BTN1_IRQ);
    NVIC_ClearPendingIRQ(CYBSP_USER_BTN2_IRQ);

    /* Initialize the interrupt and register interrupt callback */
    Cy_SysInt_Init(&sysint_cfg, &gpio_interrupt_handler);

    /* Enable the interrupt in the NVIC */
    NVIC_EnableIRQ(sysint_cfg.intrSrc);

    /* Initialize retarget-io middle-ware */
    init_retarget_io();

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("************ "
            "PSOC Edge MCU: emUSB-Device FatFs Mass Storage File System "
            "************ \r\n\n");

    /* Create the semaphore */
    button_semaphore = xSemaphoreCreateBinary();

    /* Initializes the SDHC hardware and FAT file system. */
    result = init_sdhc_fatfs();

    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Enable CM55. */
    /* CM55_APP_BOOT_ADDR must be updated if CM55 memory layout is changed.*/
    Cy_SysEnableCM55(MXCM55, CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_USEC);

    task_return = xTaskCreate(usb_task, "USB Task", RTOS_STACK_DEPTH, NULL, 
                        USB_TASK_PRIORITY, NULL);

    if (pdPASS != task_return)
    {
        handle_app_error();
    }

    task_return = xTaskCreate(button_task, "Button Task", RTOS_STACK_DEPTH, 
                        NULL, BUTTON_TASK_PRIORITY, NULL);

    if (pdPASS != task_return)
    {
        handle_app_error();
    }

    /* Prevent System deep sleep to avoid USB disconnect event. */
    mtb_hal_syspm_lock_deepsleep();

    /* Start the RTOS Scheduler */
    vTaskStartScheduler();

    /* Should never get there */
    printf("APP_LOG: Error: FreeRTOS doesn't start\r\n");

    handle_app_error();

    return 0;
}

/* [] END OF FILE */
