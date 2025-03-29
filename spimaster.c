#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
/* POSIX Header files */
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>

/* Example/Board Header files */
#include "Board.h"

#define THREADSTACKSIZE (1024)

uint16_t masterRxBuffer[1];
uint16_t masterTxBuffer[1];

void send_spi_command(uint16_t command, SPI_Handle masterSpi, SPI_Transaction transaction){
    /* Upload command value */
    masterTxBuffer[0] = command;

    /* Initialize master SPI transaction structure */
    memset((void *) masterRxBuffer, 0, 1);
    transaction.count = 1;
    transaction.txBuf = (void *) masterTxBuffer;
    transaction.rxBuf = (void *) masterRxBuffer;

    /* Toggle user LED, indicating a SPI transfer is in progress */
    GPIO_toggle(Board_GPIO_LED1);

    /* Perform SPI transfer */
    printf("Sending command: %i, ", masterTxBuffer[0]);
    bool transferOK;

    /*Set CSn to low */
    GPIO_write(IOID_11, 0);

    transferOK = SPI_transfer(masterSpi, &transaction);
    if (transferOK) {
        printf("Master received: %i \n", masterRxBuffer[0]);
    }
    else {
        printf("Unsuccessful master SPI transfer");
        while(1);
    }

    /*Set CSn to high */
    GPIO_write(IOID_11, 1);
    usleep(1000);
}



void *masterThread(void *arg0)
{
    SPI_Handle      masterSpi;
    SPI_Params      spiParams;
    SPI_Transaction transaction;

    /*Set CSn to high */
    GPIO_write(IOID_11, 1);

    /* Open SPI as master (default) */
    SPI_Params_init(&spiParams);
    spiParams.frameFormat = SPI_POL0_PHA0; //CPOL=0 and CPHA=0 required by RHD2132
    spiParams.bitRate = 115200;
    spiParams.dataSize = 16;
    spiParams.transferMode = SPI_MODE_BLOCKING;
    masterSpi = SPI_open(Board_SPI_MASTER, &spiParams);
    if (masterSpi == NULL) {
        printf("Error initializing master SPI\n");
        while (1);
    }
    else {
        printf("Master SPI initialized\n");
    }

    /* Verify the SPI connection by reading registers 40 - 44 */
    printf("Send two dummy commands to prepare the IC\n");
    send_spi_command(0b1111111100000000, masterSpi, transaction);
    send_spi_command(0b1111111100000000, masterSpi, transaction);

    printf("Reading INTAN from registers 40-44\n");
    send_spi_command(0b1110100000000000, masterSpi, transaction);
    send_spi_command(0b1110100100000000, masterSpi, transaction);
    send_spi_command(0b1110101000000000, masterSpi, transaction);
    send_spi_command(0b1110101100000000, masterSpi, transaction);
    send_spi_command(0b1110110000000000, masterSpi, transaction);

    /* Write to Register 0: ADC Configuration and Amplifier Fast Settle */
    /* Nominal settings from datasheet */
    send_spi_command(0b1000000011011110, masterSpi, transaction);

    /* Write to Register 1:  Supply Sensor and ADC Buffer Bias Current */
    /* ADC buffer bias  =  32 because we're using the lowest sampling rate*/
    send_spi_command(0b1000000101100000, masterSpi, transaction);

    /* Write to Register 2: MUX Bias Current */
    /* Nominal settings from datasheet */
    send_spi_command(0b1000001000000010, masterSpi, transaction);

    /* Write to Register 3: MUX Load, Temperature Sensor, and Auxiliary Digital Output */
    /* Disable temperature sensor and digital output*/
    send_spi_command(0b1000001100000010, masterSpi, transaction);

    /* Write to Register 4: ADC Output Format and DSP Offset Removal */
    /* Drive MISO to 1 when idle, use unsigned data instead of 2s complement, don't use absolute value, disable DSP filtering */
    send_spi_command(0b1000010011100000, masterSpi, transaction);

    /* Write to Register 5: Impedance Check Control */
    /* No electrode calibration using a DAC waveform generator */
    send_spi_command(0b1000010100000000, masterSpi, transaction);

    /* Write to Register 6: Impedance Check DAC */
    /* N/A since not using DAC */
    send_spi_command(0b1000011000000000, masterSpi, transaction);

    /* Write to Register 7: Impedance Check Amplifier Select */
    /* N/A since not using DAC */
    send_spi_command(0b1000011100000000, masterSpi, transaction);

    /* Write to Registers 8-13: On-Chip Amplifier Bandwidth Select */
    /* Upper bandwidth = 100 Hz, lower bandwidth = 0.1 Hz */
    send_spi_command(0b1000100000100110, masterSpi, transaction);
    send_spi_command(0b1000100100011010, masterSpi, transaction);
    send_spi_command(0b1000101000000101, masterSpi, transaction);
    send_spi_command(0b1000101100011111, masterSpi, transaction);
    send_spi_command(0b1000110000010000, masterSpi, transaction);
    send_spi_command(0b1000110101111100, masterSpi, transaction);

    /* Write to Registers 14-17: : Individual Amplifier Power */
    /* Power down amplifiers 8-31 */
    send_spi_command(0b1000111011111111, masterSpi, transaction);
    send_spi_command(0b1000111100000000, masterSpi, transaction);
    send_spi_command(0b1001000000000000, masterSpi, transaction);
    send_spi_command(0b1001000100000000, masterSpi, transaction);

    /* Calibrate */
    printf("Calibration \n");
    send_spi_command(0b0101010100000000, masterSpi, transaction);

    /* Nine dummy commands after calibration*/
    send_spi_command(0b1111111100000000, masterSpi, transaction);
    send_spi_command(0b1111111100000000, masterSpi, transaction);
    send_spi_command(0b1111111100000000, masterSpi, transaction);
    send_spi_command(0b1111111100000000, masterSpi, transaction);
    send_spi_command(0b1111111100000000, masterSpi, transaction);
    send_spi_command(0b1111111100000000, masterSpi, transaction);
    send_spi_command(0b1111111100000000, masterSpi, transaction);
    send_spi_command(0b1111111100000000, masterSpi, transaction);
    send_spi_command(0b1111111100000000, masterSpi, transaction);

    /* Chip is now ready, start performing conversions*/
    printf("Conversion on channel 0 \n");
    int i = 0;
    while(1){
        send_spi_command(0, masterSpi, transaction);
    }

    printf("Done");
    SPI_close(masterSpi);
    return (NULL);
}

void *mainThread(void *arg0)
{
    pthread_t           thread0;
    pthread_attr_t      attrs;
    struct sched_param  priParam;
    int                 retc;
    int                 detachState;

    GPIO_init();
    /*Set CSn to high */
    GPIO_write(IOID_11, 1);
    SPI_init();

    printf("Connecting to INTAN\n");

    /* Create application threads */
    pthread_attr_init(&attrs);

    detachState = PTHREAD_CREATE_DETACHED;
    /* Set priority and stack size attributes */
    retc = pthread_attr_setdetachstate(&attrs, detachState);
    if (retc != 0) {
        /* pthread_attr_setdetachstate() failed */
        while (1);
    }

    retc |= pthread_attr_setstacksize(&attrs, THREADSTACKSIZE);
    if (retc != 0) {
        /* pthread_attr_setstacksize() failed */
        while (1);
    }

    /* Create master thread */
    priParam.sched_priority = 1;
    pthread_attr_setschedparam(&attrs, &priParam);

    retc = pthread_create(&thread0, &attrs, masterThread, NULL);
    if (retc != 0) {
        /* pthread_create() failed */
        while (1);
    }

    return (NULL);
}
