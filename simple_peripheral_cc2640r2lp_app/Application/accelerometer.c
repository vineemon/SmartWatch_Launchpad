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

/* Example/Board Header files */
#include "Board.h"

Task_Struct myTask_spl;
Task_Struct myTask_pitch;
#define THREADSTACKSIZE    1024
uint8_t myTaskStack_spl[THREADSTACKSIZE];
uint8_t myTaskStack_pitch[THREADSTACKSIZE];
#define DRV2065_ADDR      0x5A

/* DRV2605 Registers */
#define GO                                   0x0C
#define MODE                                 0x01

uint16_t adc_value_spl;
uint16_t adc_value_pitch;
uint16_t adc_values[4];
uint16_t noise_threshold = 0;
uint16_t doctor_threshold = 0;
uint64_t pitch_count = 0;
uint64_t pitch_sum = 0;
bool first;
time_t start_time;
time_t curr_time;

extern Display_Handle dispHandle;
Semaphore_Handle adcSem;
uint16_t count;
uint8_t gate = 0;
IArg key;
ADC_Handle   adc;
uint32_t start_time;
uint32_t curr_time;

Clock_Struct clkStruct;
Clock_Handle clkHandle;
Clock_Params clkParams;

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

    ADC_Handle   adc;
    ADC_Params   params;
    ADC_Handle adc2;
    ADC_Params params2;
    uint8_t         txBuffer[2];
    uint8_t         rxBuffer[2];
    I2C_Handle      i2c;
    I2C_Params      i2cParams;
    I2C_Transaction i2cTransaction;
    Semaphore_Params semParams;


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

    /* Create I2C for usage */
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
    GPIO_setConfig(Board_GPIO_BUTTON0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* install Button callback */
    GPIO_setCallback(Board_GPIO_BUTTON0, gpioButtonFxn0);

    /* Enable interrupts */
    GPIO_enableInt(Board_GPIO_BUTTON0);

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
    while (1) {
        if (gate == 0) {
            //Semaphore_pend(adcSem, BIOS_WAIT_FOREVER);
            start_time = Clock_getTicks();
            Display_printf(dispHandle, 16, 0, "Timer value: %d\n", start_time);
            ADC_convert(adc, &adc_values[0]);
            Display_printf(dispHandle, 6, 0, "SPL Value: %d\n", adc_values[0]);
            //check if greater than speech threshold
            if (adc_values[0] > noise_threshold) {
                //doctor specified thresholds
                ADC_convert(adc, &adc_value_pitch);
                Display_printf(dispHandle, 7, 0, "Pitch Value: %d\n", adc_value_pitch);
                pitch_sum+=adc_value_pitch;
                pitch_count++;
                adc_values[1] = pitch_sum/pitch_count;
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
                if (adc_values[0] > doctor_threshold) {
                    //write to memory
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
            while ((curr_time - start_time) < 100000) {
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

//    Clock_Params_init(&clkParams);
//    clkParams.period = 1/Clock_tickPeriod;
//    clkParams.startFlag = TRUE;
//    Clock_construct(&clkStruct, myThread_spl, 1000000, &clkParams);
//    clkHandle = Clock_handle(&clkStruct);
//    Clock_start(clkHandle);

//    if (clkHandle == NULL) {
//        Display_printf(dispHandle, 19, 0, "Timer open() failed");
//        //while(1);
//    } else{
//        Display_printf(dispHandle, 19, 0, "Timer open() worked!");
//    }

    first = true;

   Task_construct(&myTask_spl, (ti_sysbios_knl_Task_FuncPtr)myThread_spl, &taskParams_spl, Error_IGNORE);
}
