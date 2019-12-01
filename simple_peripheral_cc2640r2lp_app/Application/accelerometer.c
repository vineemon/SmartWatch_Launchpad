/* Design needs:
 * 1. adc_values array should be replaced with a structure.  Which values needed at all times?
 * 2. When/how are reads from flash being triggered?  We should stop code execution and handle this.
 *    (SPIFFS is very slow at reading so it would take a while)
 * 3. Low power mode or standby mode.
 */

#include <stdint.h>
#include <stddef.h>

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/display/Display.h>
#include <ti/drivers/ADC.h>

#include <ti/drivers/I2C.h>

#include <third_party/spiffs/spiffs.h>
#include <third_party/spiffs/SPIFFSNVS.h>

/* Example/Board Header files */
#include "Board.h"

/************************************************************************************************
 * Configuration constants for SPIFFS.
 ***********************************************************************************************/
#define SPIFFS_LOGICAL_BLOCK_SIZE    (4096) //smallest block size of flash memory
#define SPIFFS_LOGICAL_PAGE_SIZE     (256) //size of a single page
#define SPIFFS_FILE_DESCRIPTOR_SIZE  (22) //size of the file descriptor

#define AMPLITUDE_SAMPLES      (44) //number of samples of amplitude written to memory at once
#define AMPLITUDE_SIZE         (AMPLITUDE_SAMPLES * 4) //number of bytes written to memory at once
                                                       //for amplitude
#define PITCH_SAMPLES          (1) //number of samples of pitch written to memory at once
#define PITCH_SIZE             (PITCH_SAMPLES * 10) //number of bytes written to memory at once for
                                                   //pitch
#define AMPLITUDE_PAGES        (2500) //maximum number of amplitude samples flash memory will hold
#define PITCH_PAGES            (25) //maximum number of pitch samples flash memory will hold

/************************************************************************************************
 * Thread stack configuration constants.
 ***********************************************************************************************/
#define THREADSTACKSIZE    1024

/************************************************************************************************
 * DR2605 configuration constants.
 ***********************************************************************************************/
#define DRV2065_ADDR      0x5A
#define GO                0x0C
#define MODE              0x01

/************************************************************************************************
 * Externs
 ***********************************************************************************************/
extern Display_Handle dispHandle;

/************************************************************************************************
 * Configuration parameters for SPIFFS.
 ***********************************************************************************************/
// SPIFFS needs RAM to perform operations on files.  It requires a work buffer
// which MUST be (2 * LOGICAL_PAGE_SIZE) bytes.
static uint8_t spiffsWorkBuffer[SPIFFS_LOGICAL_PAGE_SIZE * 2];

// The array below will be used by SPIFFS as a file descriptor cache.
static uint8_t spiffsFileDescriptorCache[SPIFFS_FILE_DESCRIPTOR_SIZE * 4];

// The array below will be used by SPIFFS as a read/write cache.
static uint8_t spiffsReadWriteCache[SPIFFS_LOGICAL_PAGE_SIZE * 2];

uint16_t amplitudeIndex = 0; //index within the amplitudeStruct for current samples

//Values below are chosen since the file descriptor is the string representation of an integer.
//These values ensure that the amplitude and pitch file descriptors don't share a name.
uint16_t amplitudePageIndex = 0; //indicates the current amplitude structure.  i.e. after 5
                                 //writes, this value would be 5.
uint16_t pitchPageIndex = AMPLITUDE_PAGES + 1; //indicates the current pitch structure. i.e. after 5 writes,
                                //this value would be AMPLITUDE_PAGES + 6.

//Holds up to AMPLTUDE_SAMPLES of amplitude data, gathered every 1 second.
//Format of amplitude data written/read from flash.
struct amplitudeStruct {
    uint16_t amplitude[AMPLITUDE_SAMPLES];
    uint16_t timeStamp[AMPLITUDE_SAMPLES];
};

//Used to write/read pitch data to/from memory once every hour.
struct pitchStruct {
    uint16_t averagePitch;
    uint16_t minimumPitch;
    uint16_t maximumPitch;
    uint16_t doctorThreshold;
    uint16_t timeStamp;
};

//Varibles needed for spiffs configuration and use
spiffs fs;
SPIFFSNVS_Data spiffsnvsData;

// Tasks
Task_Struct myTask_spl;
//Task_Struct myTask_pitch;

// Threads
uint8_t myTaskStack_spl[THREADSTACKSIZE];
//uint8_t myTaskStack_pitch[THREADSTACKSIZE];

//uint16_t adc_value_spl;
uint16_t adc_value_pitch; //the current pitch reading
uint16_t adc_values[4]; //the current pitch values for:
                        //0 -> current amplitude
                        //1 -> average pitch
                        //2 -> minimum pitch
                        //3 -> maximum pitch
uint16_t noise_threshold = 0; //amplitude threshold for voice detection
uint16_t doctor_threshold = 0; //amplitude threshold to trigger warning
uint64_t pitch_count = 0; //number of pitch samples taken since start of current hour
uint64_t pitch_sum = 0; //sum of pitch samples taken since start of current hour

bool first; //what is this for??

//used for time stamping data
time_t start_time;
uint32_t start_time;
//used for delaying main loop by 1 second
time_t curr_time;
uint32_t curr_time;
//used for writing pitch data to flash once per hour
time_t pitch_time;
uint32_t pitch_time;


/************************************************************************************************
 * Semaphore
 ***********************************************************************************************/
Semaphore_Handle adcSem;
uint16_t count;
uint8_t gate = 0;
//IArg key; //not used??
ADC_Handle   adc; //also defined in the main function??

//these only appear in commented out code?  Is that code needed, are these needed??
Clock_Struct clkStruct;
Clock_Handle clkHandle;
Clock_Params clkParams;

/************************************************************************************************
 *
 * Mimics the itoa function included in standard C/C++ libraries.  Finds the string
 * representation of an integer.
 *
 ************************************************************************************************/
void itoa(long unsigned int value, char* result, int base)
    {
      // check that the base if valid
      if (base < 2 || base > 36) { *result = '\0';}

      char* ptr = result, *ptr1 = result, tmp_char;
      int tmp_value;

      do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
      } while ( value );

      // Apply negative sign
      if (tmp_value < 0) *ptr++ = '-';
      *ptr-- = '\0';
      while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
      }
}

/********** gpioButtonFxn0 **********/
void gpioButtonFxn0(uint_least8_t index)
{
    if (count == 0) {
        gate = 1;
        GPIO_toggle(Board_GPIO_LED0);
        count++;
    } else {
        Semaphore_post(adcSem);
        gate = 0;
        GPIO_toggle(Board_GPIO_LED0);
        count = 0;
    }
}

void clkFxn(void* arg0)
{
    Semaphore_post(adcSem);
}

/********** myThread_spl **********/
void *myThread_spl(void *arg0) {

    ADC_Handle              adc;                            //ADC paramters for reading SPL
    ADC_Params              params;
    ADC_Handle              adc2;                           //not used??
    ADC_Params              params2;
    uint8_t                 txBuffer[2];                    //TX and RX buffer for I2C
    uint8_t                 rxBuffer[2];
    I2C_Handle              i2c;                            //I2C configuration parameters
    I2C_Params              i2cParams;
    I2C_Transaction         i2cTransaction;
    Semaphore_Params        semParams;                      //internal parameter for semaphores
    spiffs_file             fd;                             //SPIFFS file descriptor
    spiffs_config           fsConfig;                       //internal parameter for SPIFFS operations, not used otherwise
    int32_t                 status;                         //status of SPIFFS operations, used for error checking
    struct amplitudeStruct  amplitudeWrite;                 //holds amplitude data written to memory
    struct pitchStruct      pitchWrite;                     //holds pitch data written to memory
    char                    fileName[4];                    //SPIFFS file descriptor name


    Semaphore_Params_init(&semParams);
    adcSem = Semaphore_create(0, &semParams, Error_IGNORE);
    count = 0;
    if(adcSem == NULL)
    {
        /* Semaphore_create() failed */
        Display_print0(dispHandle, 0, 0, "adcSem Semaphore creation failed\n");
        while (1);
    }

    /* Initialize ADC and GPIO drivers */
    GPIO_init();
    ADC_init();
    I2C_init();

    /*
     * Wake up external flash on LaunchPads. It is powered off by default
     * but can be turned on by toggling the SPI chip select pin.
     */
    #ifdef Board_wakeUpExtFlash
        Board_wakeUpExtFlash();
    #endif

    /* Initialize spiffs, spiffs_config & spiffsnvsdata structures */
    status = SPIFFSNVS_config(&spiffsnvsData, Board_NVSEXTERNAL, &fs, &fsConfig,
        SPIFFS_LOGICAL_BLOCK_SIZE, SPIFFS_LOGICAL_PAGE_SIZE);
    if (status != SPIFFSNVS_STATUS_SUCCESS) {
        Display_printf(dispHandle, 0, 0,
            "Error with SPIFFS configuration.\n");

        while (1);
    }

    Display_printf(dispHandle, 0, 0, "Mounting file system...");

    /************************************************************************************************
     *
     * Setup SPIFFS for flash memory usage.
     *
     ************************************************************************************************/
    status = SPIFFS_mount(&fs, &fsConfig, spiffsWorkBuffer,
        spiffsFileDescriptorCache, sizeof(spiffsFileDescriptorCache),
        spiffsReadWriteCache, sizeof(spiffsReadWriteCache), NULL);
    if (status != SPIFFS_OK) {
        /*
         * If SPIFFS_ERR_NOT_A_FS is returned; it means there is no existing
         * file system in memory.  In this case we must unmount, format &
         * re-mount the new file system.
         */
        if (status == SPIFFS_ERR_NOT_A_FS) {
            Display_printf(dispHandle, 0, 0,
                "File system not found; creating new SPIFFS fs...");

            SPIFFS_unmount(&fs);
            status = SPIFFS_format(&fs);
            if (status != SPIFFS_OK) {
                Display_printf(dispHandle, 0, 0,
                    "Error formatting memory.\n");

                while (1);
            }

            status = SPIFFS_mount(&fs, &fsConfig, spiffsWorkBuffer,
                spiffsFileDescriptorCache, sizeof(spiffsFileDescriptorCache),
                spiffsReadWriteCache, sizeof(spiffsReadWriteCache), NULL);
            if (status != SPIFFS_OK) {
                Display_printf(dispHandle, 0, 0,
                    "Error mounting file system.\n");

                while (1);
            }
        }
        else {
            /* Received an unexpected error when mounting file system  */
            Display_printf(dispHandle, 0, 0,
                "Error mounting file system: %d.\n", status);

            while (1);
        }
    }
    /***********************************************************************************************/


    /************************************************************************************************
     *
     * I2C configuration for the haptic driver and motor.
     *
     ************************************************************************************************/
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    i2c = I2C_open(Board_I2C_TMP, &i2cParams);
    if (i2c == NULL) {
        Display_printf(dispHandle, 10, 0, "Error Initializing I2C\n");
        while (1);
    }
    else {
        Display_printf(dispHandle, 10, 0, "I2C Initialized!\n");
    }

    /* Common I2C transaction setup */
    i2cTransaction.writeBuf   = txBuffer;
    i2cTransaction.writeCount = 2;
    i2cTransaction.readBuf    = rxBuffer;
    i2cTransaction.readCount  = 0;
    i2cTransaction.slaveAddress = DRV2065_ADDR;
    /***********************************************************************************************/

    GPIO_setConfig(Board_GPIO_BUTTON0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* install Button callback */
    GPIO_setCallback(Board_GPIO_BUTTON0, gpioButtonFxn0);

    /* Enable interrupts */
    GPIO_enableInt(Board_GPIO_BUTTON0);

    //Initialize ADC
    ADC_Params_init(&params);
    ADC_Params_init(&params2);
    adc = ADC_open(Board_ADC0, &params);
    adc2 = ADC_open(Board_ADC1, &params2);
    if (adc == NULL) {
                Display_printf(dispHandle, 6, 0, "Error initializing ADC channel 0\n");
                while (1);
    }
    if (adc2 == NULL) {
            Display_printf(dispHandle, 6, 0, "Error initializing ADC channel 0\n");
            while (1);
    }

    //time since last pitch write to flash (writes once every hour)
    pitch_time = Clock_getTicks();

    //infinite loop
    while (1) {
        if (gate == 0) {
            //Semaphore_pend(adcSem, BIOS_WAIT_FOREVER);
            start_time = Clock_getTicks();
            //Display_printf(dispHandle, 16, 0, "Timer value: %d\n", start_time);

            //read amplitude value
            ADC_convert(adc, &adc_values);
            Display_printf(dispHandle, 6, 0, "SPL Value: %d\n", adc_values[0]);

            //check if amplitude greater than speech threshold
            if (adc_values[0] > noise_threshold) {
                //read pitch value
                ADC_convert(adc, &adc_value_pitch);
                //adc_values.pitch = adc_value_pitch;
                Display_printf(dispHandle, 7, 0, "Pitch Value: %d\n", adc_value_pitch);

                //update running sum and average of pitch values over last hour
                pitch_sum+=adc_value_pitch;
                pitch_count++;
                adc_values[1] = pitch_sum/pitch_count;

                //set minimum and maximum pitch values if needed
                //first is true for the first sample each hour
                if (first) {
                    adc_values[2] = adc_value_pitch;
                    adc_values[3] = adc_value_pitch;
                    first = false;
                }
                else if (adc_value_pitch < adc_values[2]) {
                    adc_values[2] = adc_value_pitch;
                }
               else if (adc_value_pitch > adc_values[3]) {
                    adc_values[3] = adc_value_pitch;
                }
                Display_printf(dispHandle, 8, 0, "Average Pitch value: %d\n", adc_values[1]);
                Display_printf(dispHandle, 9, 0, "Min Pitch value: %d\n", adc_values[2]);
                Display_printf(dispHandle, 10, 0, "Max Pitch value: %d\n", adc_values[3]);
                Display_printf(dispHandle, 20, 0, "Doctor Threshold: %d\n", doctor_threshold);

                //if hour elapsed since last pitch write to flash, write current values to flash and reset the hourly data
                if ((start_time - pitch_time) > (100000 * 60 * 60)) {
                    pitchWrite.averagePitch = adc_values[1];
                    pitchWrite.minimumPitch = adc_values[2];
                    pitchWrite.maximumPitch = adc_values[3];
                    pitchWrite.doctorThreshold = doctor_threshold;
                    pitchWrite.timeStamp = start_time / 100000;

                    itoa(pitchPageIndex, &fileName, 10);
                    fd = SPIFFS_open(&fs, &fileName, SPIFFS_RDWR, 0);
                    if (fd >= 0) {
                        Display_printf(dispHandle, 15, 40, "Removing spiffsFile for pitch: %d\n", pitchPageIndex);
                        while (SPIFFS_fremove(&fs, fd) != SPIFFS_OK) {
                            Display_printf(dispHandle, 19, 40, "Error removing spiffsFile for pitch. %d %d\n", pitchPageIndex, start_time / 100000);
                        }
                        SPIFFS_close(&fs, fd);
                        fd = SPIFFS_open(&fs, &fileName, SPIFFS_RDWR, 0);
                    }
                    Display_printf(dispHandle, 14, 40, "Creating spiffsFile for pitch: %d\n", pitchPageIndex);
                    fd = SPIFFS_open(&fs, &fileName, SPIFFS_CREAT | SPIFFS_RDWR, 0);
                    while (fd < 0) {
                        Display_printf(dispHandle, 16, 40, "Error creating spiffsFile for pitch. %d %d\n", pitchPageIndex, start_time / 100000);
                        fd = SPIFFS_open(&fs, &fileName, SPIFFS_CREAT | SPIFFS_RDWR, 0);
                    }
                    Display_printf(dispHandle, 17, 40, "Writing to spiffsFile for pitch: %d\n", pitchPageIndex);
                    if (SPIFFS_write(&fs, fd, (void *) &pitchWrite, PITCH_SIZE) < 0) {
                        Display_printf( dispHandle, 21, 40, "Error writing spiffsFile for pitch: %d\n", pitchPageIndex);
                    }
                    SPIFFS_close(&fs, fd);
                    pitch_time = Clock_getTicks();
                    pitchPageIndex++;

                    first = true;
                    pitch_sum = 0;
                    pitch_count = 0;
                    if (pitchPageIndex > (AMPLITUDE_PAGES + PITCH_PAGES)) {
                        pitchPageIndex = AMPLITUDE_PAGES + 1;
                    }
                }

                //if ampitude greater than set doctor threshold, write current amplitude reading to flash and trigger haptic motor user alert
                if (adc_values[0] > doctor_threshold) {
                    //write to memory
                    amplitudeWrite.amplitude[amplitudeIndex] = adc_values[0];
                    amplitudeWrite.timeStamp[amplitudeIndex] = start_time / 100000;
                    Display_printf(dispHandle, 11, 0, "Amplitude Index: %d\n", amplitudeIndex);
                    Display_printf(dispHandle, 12, 0, "Amplitude Value: %d\n", adc_values[0]);
                    Display_printf(dispHandle, 13, 0, "Time Stamp: %d\n", start_time / 100000);

                    if (amplitudeIndex > AMPLITUDE_SAMPLES - 1) {
                        itoa(amplitudePageIndex, &fileName, 10);
                        fd = SPIFFS_open(&fs, &fileName, SPIFFS_RDWR, 0);
                        if (fd >= 0) {
                            Display_printf(dispHandle, 15, 0, "Removing spiffsFile: %d\n", amplitudePageIndex);
                            while (SPIFFS_fremove(&fs, fd) != SPIFFS_OK) {
                                Display_printf(dispHandle, 19, 0, "Error removing spiffsFile. %d %d\n", amplitudePageIndex, start_time / 100000);
                            }
                            SPIFFS_close(&fs, fd);
                            fd = SPIFFS_open(&fs, &fileName, SPIFFS_RDWR, 0);
                        }
                        Display_printf(dispHandle, 14, 0, "Creating spiffsFile: %d\n", amplitudePageIndex);
                        fd = SPIFFS_open(&fs, &fileName, SPIFFS_CREAT | SPIFFS_RDWR, 0);
                        while (fd < 0) {
                            Display_printf(dispHandle, 16, 0, "Error creating spiffsFile. %d %d\n", amplitudePageIndex, start_time / 100000);
                            fd = SPIFFS_open(&fs, &fileName, SPIFFS_CREAT | SPIFFS_RDWR, 0);
                        }
                        Display_printf(dispHandle, 17, 0, "Writing to spiffsFile: %d\n", amplitudePageIndex);
                        if (SPIFFS_write(&fs, fd, (void *) &amplitudeWrite, AMPLITUDE_SIZE) < 0) {
                            Display_printf( dispHandle, 21, 0, "Error writing spiffsFile: %d\n", amplitudePageIndex);
                        }
                        //Display_printf(dispHandle, 18, 0, "Write Success: %d\n", amplitudePageIndex);
                        SPIFFS_close(&fs, fd);
                        amplitudePageIndex++;
                        amplitudeIndex = 0;
                        if (amplitudePageIndex > AMPLITUDE_PAGES) {
                            amplitudePageIndex = 0;
                        }
                    }
                    amplitudeIndex++;
                    //haptic write
                    txBuffer[0] = MODE;
                    txBuffer[1] = 0x00;
                    if(!I2C_transfer(i2c, &i2cTransaction)) {
                        Display_printf(dispHandle, 15, 0, "Error. No Haptic Driver found!");
                        while(1);
                    }
                    txBuffer[0] = GO;
                    txBuffer[1] = 0x01;
                    I2C_transfer(i2c, &i2cTransaction);
                    Display_printf(dispHandle, 12, 0, "Above Doctor Threshold");
                } else {
                    Display_printf(dispHandle, 12, 0, "Not within range!");
                }
            }
            curr_time = Clock_getTicks();
            //delay for 1 second
            while ((curr_time - start_time) < 10) { //1 second is 100000, value temporarily lowered for testing
                curr_time = Clock_getTicks();
            }
        } else {
            /* Pend on semaphore, tmp116Sem */
            /* Blocking mode conversion */
            Semaphore_pend(adcSem, BIOS_WAIT_FOREVER);
        }
    }
}

/********** myThread_create **********/
void myThread_create(void) {
    Task_Params taskParams_spl;

    // Configure task
    Task_Params_init(&taskParams_spl);
    taskParams_spl.stack = myTaskStack_spl;
    taskParams_spl.stackSize = THREADSTACKSIZE;
    taskParams_spl.priority = 1;
    first = true;

   Task_construct(&myTask_spl, (ti_sysbios_knl_Task_FuncPtr)myThread_spl, &taskParams_spl, Error_IGNORE);
}
