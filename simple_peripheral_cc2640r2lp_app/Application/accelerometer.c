#include <stdint.h>
#include <stddef.h>

#include <xdc/std.h>
#include <xdc/runtime/Error.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>

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
uint16_t noise_threshold;
uint16_t doctor_threshold;

extern Display_Handle dispHandle;
Semaphore_Handle adcSem;
uint16_t count;
uint8_t gate = 0;
IArg key;
ADC_Handle   adc;

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

/********** myThread_spl **********/
void *myThread_spl(void *arg0) {

    ADC_Handle   adc;
    ADC_Params   params;
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
        Display_printf(dispHandle, 0, 0, "Error Initializing I2C\n");
        while (1);
    }
    else {
        Display_printf(dispHandle, 0, 0, "I2C Initialized!\n");
    }
    /* Common I2C transaction setup */
    i2cTransaction.writeBuf   = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf    = rxBuffer;
    i2cTransaction.readCount  = 2;
    i2cTransaction.slaveAddress = DRV2065_ADDR;
    GPIO_setConfig(Board_GPIO_BUTTON0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* install Button callback */
    GPIO_setCallback(Board_GPIO_BUTTON0, gpioButtonFxn0);

    /* Enable interrupts */
    GPIO_enableInt(Board_GPIO_BUTTON0);

    ADC_Params_init(&params);
    adc = ADC_open(Board_ADC0, &params);

    if (adc == NULL) {
            Display_printf(dispHandle, 6, 0, "Error initializing ADC channel 0\n");
            while (1);
    }

    while (1) {
        if (gate == 0) {
            ADC_convert(adc, &adc_value_spl);
            Display_printf(dispHandle, 6, 0, "SPL Value: %d\n", adc_value_spl);
            //check if greater than speech threshold
            if (adc_value_spl > noise_threshold) {
                //doctor specified thresholds

                //write to memory

                //haptic write
                txBuffer[0] = MODE;
                txBuffer[1] = 0x00;
                I2C_transfer(i2c, &i2cTransaction);
                txBuffer[0] = GO;
                txBuffer[1] = 1;
                I2C_transfer(i2c, &i2cTransaction);
            }
        } else {
            /* Pend on semaphore, tmp116Sem */
            /* Blocking mode conversion */
            Semaphore_pend(adcSem, BIOS_WAIT_FOREVER);
        }
    }
}

void *myThread_pitch(void *arg0) {
    ADC_Handle   adc;
    ADC_Params   params;
    adc = ADC_open(Board_ADC1, &params);
    if (adc == NULL) {
            Display_printf(dispHandle, 6, 0, "Error initializing ADC channel 0\n");
            while (1);
    }
    while (1) {
            if (gate == 0) {
                ADC_convert(adc, &adc_value_pitch);
                Display_printf(dispHandle, 6, 0, "Pitch value: %d\n", adc_value_pitch);
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
    Task_Params taskParams_pitch;

    // Configure task
    Task_Params_init(&taskParams_spl);
    taskParams_spl.stack = myTaskStack_spl;
    taskParams_spl.stackSize = THREADSTACKSIZE;
    taskParams_spl.priority = 1;

    // Configure task
    Task_Params_init(&taskParams_pitch);
    taskParams_pitch.stack = myTaskStack_pitch;
    taskParams_pitch.stackSize = THREADSTACKSIZE;
    taskParams_pitch.priority = 1;

   Task_construct(&myTask_spl, myThread_spl, &taskParams_spl, Error_IGNORE);
   Task_construct(&myTask_pitch, myThread_pitch, &taskParams_pitch, Error_IGNORE);
}
