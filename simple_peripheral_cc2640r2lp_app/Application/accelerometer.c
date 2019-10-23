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

/* Example/Board Header files */
#include "Board.h"

Task_Struct myTask;
#define THREADSTACKSIZE    1024
uint8_t myTaskStack[THREADSTACKSIZE];

uint16_t adcValue;
uint16_t threshold;

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

/********** myThread **********/
void *myThread(void *arg0) {

    ADC_Handle   adc;
    ADC_Params   params;
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
            ADC_convert(adc, &adcValue);
            Display_printf(dispHandle, 6, 0, "ADC value: %d\n", adcValue);
            //check if greater than speech threshold
            if (adcValue > threshold) {
                //doctor specified thresholds

                //write to memory

                //haptic write

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
    Task_Params taskParams;

    // Configure task
    Task_Params_init(&taskParams);
    taskParams.stack = myTaskStack;
    taskParams.stackSize = THREADSTACKSIZE;
    taskParams.priority = 1;

   Task_construct(&myTask, myThread, &taskParams, Error_IGNORE);
}
