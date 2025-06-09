//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - Timer Demo
// Application Overview - This application is to showcases the usage of Timer
//                        DriverLib APIs. The objective of this application is
//                        to showcase the usage of 16 bit timers to generate
//                        interrupts which in turn toggle the state of the GPIO
//                        (driving LEDs).
//                        Two timers with different timeout value(one is twice
//                        the other) are set to toggle two different GPIOs which
//                        in turn drives two different LEDs, which will give a
//                        blinking effect.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup timer_demo
//! @{
//
//*****************************************************************************

// Standard include
#include <stdio.h>

// SimpleLink includes
#include "simplelink.h"

// Driverlib includes
#include "hw_types.h"
#include "interrupt.h"
#include "hw_ints.h"
#include "hw_apps_rcm.h"
#include "hw_common_reg.h"
#include "prcm.h"
#include "spi.h"
#include "rom.h"
#include "rom_map.h"
#include "hw_memmap.h"
#include "timer.h"
#include "utils.h"

// Common interface includes
#include "./timer_if.h"
#include "gpio_if.h"

// Added for GPIO Interrupt
#include "gpio.h"
#include "common.h"

// I2C included libraries
#include "i2c_if.h"

#include "pin_mux_config.h"

// Added libraries
#include "uart_if.h"
#include "uart.h"
#include "stdint.h"

// Common interface includes
#include "pin_mux_config.h"
#include "Adafruit_SSD1351.h"
#include "Adafruit_GFX.h"
#include "oled_test.h"
#include "glcdfont.h"

// Custom includes
#include "utils/network_utils.h"

#define CHAR_WIDTH     5
#define CHAR_HEIGHT    7
#define CHAR_SPACING   1
#define OLED_WIDTH     128
#define OLED_HEIGHT    128

#define MIN     (4 + 1)
#define MAX     (OLED_HEIGHT - 4 - 1)

//*****************************************************************************
//                      AWS Setup
//*****************************************************************************
//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                20    /* Current Date */
#define MONTH               5     /* Month 1-12 */
#define YEAR                2025  /* Current year */
#define HOUR                15    /* Time - hours */
#define MINUTE              15    /* Time - minutes */
#define SECOND              0     /* Time - seconds */


#define APPLICATION_NAME      "SSL"
#define APPLICATION_VERSION   "SQ24"
#define SERVER_NAME           "as9avp48j9bhe-ats.iot.us-east-1.amazonaws.com"
#define GOOGLE_DST_PORT       8443



#define POSTHEADER  "POST /things/HeatherCC3200/shadow HTTP/1.1\r\n"
#define GETHEADER   "GET /things/HeatherCC3200/shadow HTTP/1.1\r\n"
#define HOSTHEADER  "Host: as9avp48j9bhe-ats.iot.us-east-1.amazonaws.com\r\n"


#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"

#define DATA1 "{" \
            "\"state\": {\r\n"                                              \
                "\"desired\" : {\r\n"                                       \
                    "\"var\" :\""                                           \
                        "Hello phone, "                                     \
                        "message from CC3200 via AWS IoT!"                  \
                        "\"\r\n"                                            \
                "}"                                                         \
            "}"                                                             \
        "}\r\n\r\n"
//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define APPLICATION_VERSION        "1.4.0"
#define FOREVER                    1

//*****************************************************************************
//
// Application Master/Slave mode selector macro
//
// MASTER_MODE = 1 : Application in master mode
// MASTER_MODE = 0 : Application in slave mode
//
//*****************************************************************************
#define MASTER_MODE      1

#define SPI_IF_BIT_RATE  100000
#define TR_BUFF_SIZE     100

#define MASTER_MSG       "This is CC3200 SPI Master Application\n\r"
#define SLAVE_MSG        "This is CC3200 SPI Slave Application\n\r"

//*****************************************************************************
//                      Global Variables for Vector Table
//*****************************************************************************
static unsigned char g_ucTxBuff[TR_BUFF_SIZE];
static unsigned char g_ucRxBuff[TR_BUFF_SIZE];
static unsigned char ucTxBuffNdx;
static unsigned char ucRxBuffNdx;


#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

//****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//****************************************************************************
static int set_time();
static void BoardInit(void);
static int http_post(int);
static int http_get(int);

//*****************************************************************************
//                      Global Variables for Functionality
//*****************************************************************************
// long long is 64 bits long and the input signal is 48 bits + 1 bit at the end
static uint64_t bitStream = 0;
//static uint64_t bitMask = (1ULL << 49);
static volatile unsigned long startTime = 0;
// maybe change startTime to a unsigned long like the counter


static char global_buf_copy[65] = {0};

//*****************************************************************************
//
// Globals used by the timer interrupt handler.
//
//*****************************************************************************
static volatile unsigned long g_ulSysTickValue;
static volatile unsigned long g_ulBase;
static volatile unsigned long g_ulGPIOBase;
//static volatile unsigned long g_ulGPIOTimerInts = 0;
static volatile unsigned long g_ulIntClearVector;
static volatile unsigned long g_ulCounter = 0;



volatile int finish = 0;
static volatile int counter = 0;
volatile char buf_char = '\0';

volatile char* bufStr = "Undefined";
volatile int sameFlag = 0;
volatile int pxFlag = 0;
volatile unsigned long delay_check = 0;
volatile unsigned long delayTime = 0;


static volatile int tap = 0;
volatile uint64_t prev_buf = 0;

static char  buf_str[65] = {0};
static int   idx;

// Buffer for the creature list
static char creatures[128] = "[";  // Start JSON array
static int firstCreature = 1;  // Flag to manage commas


//*****************************************************************************
//
// Global Hash Table for Unique ID For Remote Control
//
//*****************************************************************************

static uint64_t BUT_0 = 0x1A00200804C4C;
static uint64_t BUT_1 = 0x1A00200800404;
static uint64_t BUT_2 = 0x1A00200804444;
static uint64_t BUT_3 = 0x1A00200802424;
static uint64_t BUT_4 = 0x1A00200806464;
static uint64_t BUT_5 = 0x1A00200801414;
static uint64_t BUT_6 = 0x1A00200805454;
static uint64_t BUT_7 = 0x1A00200803434;
static uint64_t BUT_8 = 0x1A00200807474;
static uint64_t BUT_9 = 0x1A00200800C0C;
static uint64_t MUTE = 0x1A00200802626;
static uint64_t LAST = 0x1A00200807676;


static char CHAR_0 = 32;
static char CHAR_2 = 'A';
static char CHAR_3 = 'D';
static char CHAR_4 = 'G';
static char CHAR_5 = 'J';
static char CHAR_6 = 'M';
static char CHAR_7 = 'P';
static char CHAR_8 = 'T';
static char CHAR_9 = 'W';
static char CHAR_MUTE = '0'; // Send
static char CHAR_LAST = '1'; // Delete


//*****************************************************************************
//
//! SPI Slave Interrupt handler
//!
//! This function is invoked when SPI slave has its receive register full or
//! transmit register empty.
//!
//! \return None.
//
//*****************************************************************************
static void SlaveIntHandler()
{
    unsigned long ulRecvData;
    unsigned long ulStatus;

    ulStatus = MAP_SPIIntStatus(GSPI_BASE,true);

    MAP_SPIIntClear(GSPI_BASE,SPI_INT_RX_FULL|SPI_INT_TX_EMPTY);

    if(ulStatus & SPI_INT_TX_EMPTY)
    {
        MAP_SPIDataPut(GSPI_BASE,g_ucTxBuff[ucTxBuffNdx%TR_BUFF_SIZE]);
        ucTxBuffNdx++;
    }

    if(ulStatus & SPI_INT_RX_FULL)
    {
        MAP_SPIDataGetNonBlocking(GSPI_BASE,&ulRecvData);
        g_ucTxBuff[ucRxBuffNdx%TR_BUFF_SIZE] = ulRecvData;
        Report("%c",ulRecvData);
        ucRxBuffNdx++;
    }
}

//*****************************************************************************
//
//! SPI Master mode main loop
//!
//! This function configures SPI modelue as master and enables the channel for
//! communication
//!
//! \return None.
//
//*****************************************************************************
void MasterMain()
{

    //unsigned long ulUserData;
    //unsigned long ulDummy;

    //
    // Initialize the message
    //
    memcpy(g_ucTxBuff,MASTER_MSG,sizeof(MASTER_MSG));

    //
    // Set Tx buffer index
    //
    ucTxBuffNdx = 0;
    ucRxBuffNdx = 0;

    //
    // Reset SPI
    //
    MAP_SPIReset(GSPI_BASE);

    //
    // Configure SPI interface
    //
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                     SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                     (SPI_SW_CTRL_CS |
                     SPI_4PIN_MODE |
                     SPI_TURBO_OFF |
                     SPI_CS_ACTIVEHIGH |
                     SPI_WL_8));

    //
    // Enable SPI for communication
    //
    MAP_SPIEnable(GSPI_BASE);

//    //
//    // Print mode on uart
//    //
//    Message("Enabled SPI Interface in Master Mode\n\r");
//
//    //
//    // User input
//    //
//    Report("Press any key to transmit data....");

    //
    // Read a character from UART terminal
    //
    //ulUserData = MAP_UARTCharGet(UARTA0_BASE); // STOPS HERE


    //
    // Send the string to slave. Chip Select(CS) needs to be
    // asserted at start of transfer and deasserted at the end.
    //
    MAP_SPITransfer(GSPI_BASE,g_ucTxBuff,g_ucRxBuff,50,
            SPI_CS_ENABLE|SPI_CS_DISABLE);

//    //
//    // Report to the user
//    //
//    Report("\n\rSend      %s",g_ucTxBuff);
//    Report("Received  %s",g_ucRxBuff);
//
//    //
//    // Print a message
//    //
//    Report("\n\rType here (Press enter to exit) :");

}

//*****************************************************************************
//
//! SPI Slave mode main loop
//!
//! This function configures SPI modelue as slave and enables the channel for
//! communication
//!
//! \return None.
//
//*****************************************************************************
void SlaveMain()
{
    //
    // Initialize the message
    //
    memcpy(g_ucTxBuff,SLAVE_MSG,sizeof(SLAVE_MSG));

    //
    // Set Tx buffer index
    //
    ucTxBuffNdx = 0;
    ucRxBuffNdx = 0;

    //
    // Reset SPI
    //
    MAP_SPIReset(GSPI_BASE);

    //
    // Configure SPI interface
    //
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                     SPI_IF_BIT_RATE,SPI_MODE_SLAVE,SPI_SUB_MODE_0,
                     (SPI_HW_CTRL_CS |
                     SPI_4PIN_MODE |
                     SPI_TURBO_OFF |
                     SPI_CS_ACTIVEHIGH |
                     SPI_WL_8));

    //
    // Register Interrupt Handler
    //
    MAP_SPIIntRegister(GSPI_BASE,SlaveIntHandler);

    //
    // Enable Interrupts
    //
    MAP_SPIIntEnable(GSPI_BASE,SPI_INT_RX_FULL|SPI_INT_TX_EMPTY);

    //
    // Enable SPI for communication
    //
    MAP_SPIEnable(GSPI_BASE);

}

//*****************************************************************************
//
//!       Displays 8 Vertical Lines with different colors
//
//*****************************************************************************
void eightVLines() {
    fillScreen(BLACK);
    drawFastVLine( 10, 0, OLED_HEIGHT, RED);
    drawFastVLine( 25, 0, OLED_HEIGHT, 0xFD20); // ORANGE
    drawFastVLine( 40, 0, OLED_HEIGHT, YELLOW);
    drawFastVLine( 55, 0, OLED_HEIGHT, GREEN);
    drawFastVLine( 70, 0, OLED_HEIGHT, BLUE);
    drawFastVLine( 85, 0, OLED_HEIGHT, MAGENTA);
    drawFastVLine( 100, 0, OLED_HEIGHT, 0x8010); //PURPLE
    drawFastVLine( 115, 0, OLED_HEIGHT, WHITE);
}

//*****************************************************************************
//
//!       Displays RECEIVING MESSAGE
//
//*****************************************************************************
void printCharArray64(char arr[64]) {
    int x = 0;
    int y = 64;
    int i = 0;

    for (i = 0; i < 64; i++) {
        if (arr[i] == '\0') break;  // Stop if null terminator is found

        drawChar(x, y, arr[i], WHITE, BLACK, 1);
        x += CHAR_WIDTH + CHAR_SPACING;

        if (x + CHAR_WIDTH > OLED_WIDTH) {
            x = 0;
            y += CHAR_HEIGHT + 1;
        }

        if (y + CHAR_HEIGHT > OLED_HEIGHT) {
            break;  // Stop if screen overflow
        }
    }
}

//*****************************************************************************
//
//! The interrupt handler for the first timer interrupt.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************
void
TimerBaseIntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    Timer_IF_InterruptClear(g_ulBase);

    g_ulCounter++;

    //Toggles green light
    //GPIO_IF_LedToggle(MCU_GREEN_LED_GPIO);
}

//*****************************************************************************
//
//! The interrupt handler for the GPIO interrupt.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************
static void
GPIOA0IntHandler(void)
{
    // Clear Flag
    //MAP_GPIOIntClear(GPIOA0_BASE, 0x40);

    // grab current bit value of GPIO pin
    // Print status 1 - 2, 0 - 0
    //unsigned char status = GPIO_IF_Get(61, GPIOA0_BASE, 0x40);

    //
    // Clear the timer interrupt. <-- don't need because not a timer interrupt
    //
    //Timer_IF_InterruptClear(g_ulGPIOBase);


    // Print status 1 - 64, 0 - 0
//    Report("Status = %d\n", status);
//
//    // Read signal bit
//
    unsigned long status = GPIOPinRead(GPIOA0_BASE, 0x40);


    // RISING
    if (status) {
        startTime = g_ulCounter;
        MAP_GPIOIntClear(GPIOA0_BASE, 0x40);
     }

    else{
        unsigned long pulse = g_ulCounter - startTime;

        // reset signal
        if (pulse > 80) {
            bitStream = 0;
            counter = 0;
        }

        //
        // Decoding
        //
        if (pulse < 7) {
            bitStream <<= 1;
            counter++;
        }

        if (pulse > 10) {
            bitStream |= 1;
            bitStream <<= 1;
            counter++;
        }


        // Output Ready!
        if (counter == 48){

            unsigned long delay = g_ulCounter - delayTime;
            delay_check = delay;

            // Update Buffer Char
            if (bitStream == BUT_2) {buf_char = CHAR_2;}
            if (bitStream == BUT_3) {buf_char = CHAR_3;}
            if (bitStream == BUT_4) {buf_char = CHAR_4;}
            if (bitStream == BUT_5) {buf_char = CHAR_5;}
            if (bitStream == BUT_6) {buf_char = CHAR_6;}
            if (bitStream == BUT_7) {buf_char = CHAR_7;}
            if (bitStream == BUT_8) {buf_char = CHAR_8;}
            if (bitStream == BUT_9) {buf_char = CHAR_9;}

            if (bitStream == BUT_0) {buf_char = CHAR_0;}
            if (bitStream == MUTE) {buf_char = CHAR_MUTE;}
            if (bitStream == LAST) {buf_char = CHAR_LAST;}

            // No Previous Button
            if (!prev_buf) {
                delayTime = g_ulCounter; // unsure how to start
                prev_buf = bitStream;

                finish = 1;
            }

            else {
                //unsigned long delay = g_ulCounter - delayTime;


                // Delay Threshold: 2 seconds
                if (delay < 20000) {

                    if (prev_buf == bitStream) {

                        // Special Buttons
                        if (bitStream == LAST || bitStream == BUT_0 || bitStream == MUTE) {
                            pxFlag = 0;
                        }
                        else {
                            pxFlag = 1;
                        }

                        tap++;

                        // Map Logic
                        if ((bitStream == BUT_2) && (tap < 3)) {buf_char = CHAR_2 + tap;}
                        else if (bitStream == BUT_2) {buf_char = CHAR_2 + 2;}

                        if ((bitStream == BUT_3) && (tap < 3)) {buf_char = CHAR_3 + tap;}
                        else if (bitStream == BUT_3) {buf_char = CHAR_3 + 2;}

                        if ((bitStream == BUT_4) && (tap < 3)) {buf_char = CHAR_4 + tap;}
                        else if (bitStream == BUT_4) {buf_char = CHAR_4 + 2;}

                        if ((bitStream == BUT_5) && (tap < 3)) {buf_char = CHAR_5 + tap;}
                        else if (bitStream == BUT_5) {buf_char = CHAR_5 + 2;}

                        if ((bitStream == BUT_6) && (tap < 3)) {buf_char = CHAR_6 + tap;}
                        else if (bitStream == BUT_6) {buf_char = CHAR_6 + 2;}

                        if ((bitStream == BUT_7) && (tap < 4)) {buf_char = CHAR_7 + tap;}
                        else if (bitStream == BUT_7) {buf_char = CHAR_7 + 3;}

                        if ((bitStream == BUT_8) && (tap < 3)) {buf_char = CHAR_8 + tap;}
                        else if (bitStream == BUT_8) {buf_char = CHAR_8 + 2;}

                        if ((bitStream == BUT_9) && (tap < 4)) {buf_char = CHAR_9 + tap;}
                        else if (bitStream == BUT_9) {buf_char = CHAR_9 + 3;}


                        finish = 1;

                    }

                    // New Button
                    else {
                        delayTime = g_ulCounter;
                        prev_buf = bitStream;
                        tap = 0;
                        pxFlag = 0;
                        finish = 1;
                    }

                }
                // Delay Threshold Exceeded
                else {
                    delayTime = g_ulCounter;
                    prev_buf = bitStream;
                    tap = 0;
                    pxFlag = 0;

                    finish = 1;
                }

            }

        }


        MAP_GPIOIntClear(GPIOA0_BASE, 0x40);
    }



}

//*****************************************************************************
//
//! UART Interrupt Handler
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
char receiverBuffer[64];
volatile int recIndex = 0;
volatile unsigned int printFlag = 0;

static void
UART_Handler(void) {
    //printf("Receiving... \n");

    // check status
    unsigned long status = UARTIntStatus(UARTA1_BASE, true);

    // clear interrupt
    UARTIntClear(UARTA1_BASE, status);

    while (UARTCharsAvail(UARTA1_BASE)) {
        char c = MAP_UARTCharGetNonBlocking(UARTA1_BASE);

        if (recIndex == 0) {
            fillRect(0, 64, 128, 64, BLACK);
        }

        if (c == '\n') {
            receiverBuffer[recIndex] = '\0';
            printFlag = 1;
            recIndex = 0;
        }

        else {
            receiverBuffer[recIndex++] = c;
            recIndex %= 64;
        }


    }

}

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

//*****************************************************************************
//
//!  UART Initalizer
//!
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************
static void
InitUart(){
    // UART Interrupt Enable Code
    PRCMPeripheralReset(PRCM_UARTA1);

    // Configure UART1 With Desired Baud Rate
    UARTConfigSetExpClk(
        UARTA1_BASE,
        PRCMPeripheralClockGet(PRCM_UARTA1),
        115200,
        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE)
    );

    UARTIntRegister(UARTA1_BASE, UART_Handler);

    UARTIntEnable(UARTA1_BASE, UART_INT_RX);

    // Enable FIFOs for Buffering
    UARTFIFOLevelSet(UARTA1_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);

    // Enable UART Module
    UARTEnable(UARTA1_BASE);

}

//*****************************************************************************
//
//! This function updates the date and time of CC3200.
//!
//! \param None
//!
//! \return
//!     0 for success, negative otherwise
//!
//*****************************************************************************

static int set_time() {
    long retVal;

    g_time.tm_day  = DATE;
    g_time.tm_mon  = MONTH;
    g_time.tm_year = YEAR;
    g_time.tm_hour = HOUR;
    g_time.tm_min  = MINUTE;
    g_time.tm_sec  = SECOND;

    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                          SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                          sizeof(SlDateTime),(unsigned char *)(&g_time));

    ASSERT_ON_ERROR(retVal);
    return SUCCESS;
}


//*****************************************************************************
//
//!       Displays "Hello World!"
//
//*****************************************************************************
void printString(char* message) {
    int x = 64 - (CHAR_WIDTH * 8);
    int y = 64;
    int i = 0;
    for (i = 0; message[i] != '\0'; i++) {
        drawChar(x, y, message[i], WHITE, BLACK, 1);
        x += CHAR_WIDTH + CHAR_SPACING;

        if (x + CHAR_WIDTH > OLED_WIDTH) {
            x = 0;
            y += CHAR_HEIGHT + 1;
        }

        if (y + CHAR_HEIGHT > OLED_HEIGHT) {
            break;
        }
    }
}

//*****************************************************************************
//
//!       CREATURE CATCH - THROW ACCELEROMETER
//
//*****************************************************************************
int creatureCatch() {

    unsigned char ucDevAddr, ucRegOffset, ucRdLen;
    unsigned char aucRdDataBuf[256];

    ucDevAddr = 0x18;  // register
    ucRegOffset = 0x2; // offset
    ucRdLen = 6; // 6 bytes (6 values)

    signed char x;
    signed char y;
    signed char z;

    int throw_flag = 0;
    int throw_score = 0;

    int counter = 0;
    int maxVal = 0;

    int throw_data[200];
    int i = 0;
    printf("start collecting\n");

    fillScreen(BLACK);
    char* message = "--- THROW ---";
    printString(message);


    while(!throw_flag){
        I2C_IF_Write(ucDevAddr,&ucRegOffset,1,0);
        I2C_IF_Read(ucDevAddr, &aucRdDataBuf[0], ucRdLen);

        x = aucRdDataBuf[1]; // X Raw Data
        y = aucRdDataBuf[3]; // Y Raw Data
        z = aucRdDataBuf[5]; // Z Raw Data

        UART_PRINT("X = %d, Y = %d, z = %d\n\r", x, y, z);

        // Check if a throw
        if(x < 0){
            if(counter == 200){
                counter = 0;
                // Computer array
                maxVal = throw_data[0];
                for(i = 0; i < 20; i++){
                    if(throw_data[i] > maxVal){
                        maxVal = throw_data[i];
                    }
                }
                //printf("Max throwing value = %d\n", maxVal);
                throw_flag = 1;
            }
            else{
                // STORE
                if(x > -20){
                    throw_score = 1;
                    //UART_PRINT("Bad Throw\n");
                }
                else if(x < -20 && x > -50){
                    throw_score = 2;
                    //UART_PRINT("Medium Throw\n");
                }
                else if(x < -50){
                    throw_score = 3;
                    //UART_PRINT("Good Throw\n");
                }
                throw_data[counter] = throw_score;
                counter++;
            }

        }
    }
    //printf("Max throwing value = %d\n", maxVal);
    return maxVal;

}

//*****************************************************************************
//
//!       ADD CREATURE TO COLLECTION --> AWS
//
//*****************************************************************************
void createCharacter() {
    char* message1 = "Create Character";
    char* message2 = "Name: ";

    fillScreen(BLACK);
    int x = 64 - (CHAR_WIDTH * 8);
    int y = 64 - CHAR_HEIGHT;
    int i = 0;
    for (i = 0; message1[i] != '\0'; i++) {
        drawChar(x, y, message1[i], WHITE, BLACK, 1);
        x += CHAR_WIDTH + CHAR_SPACING;

        if (x + CHAR_WIDTH > OLED_WIDTH) {
            x = 0;
            y += CHAR_HEIGHT + 1;
        }

        if (y + CHAR_HEIGHT > OLED_HEIGHT) {
            break;
        }
    }

    x = 64 - (CHAR_WIDTH * 8);
    y = 64 + (CHAR_HEIGHT / 2);
    i = 0;
    for (i = 0; message2[i] != '\0'; i++) {
        drawChar(x, y, message2[i], WHITE, BLACK, 1);
        x += CHAR_WIDTH + CHAR_SPACING;

        if (x + CHAR_WIDTH > OLED_WIDTH) {
            x = 0;
            y += CHAR_HEIGHT + 1;
        }

        if (y + CHAR_HEIGHT > OLED_HEIGHT) {
            break;
        }
    }


//    x = 64 - (CHAR_WIDTH * 2);
}

void addCreature(const char* name) {
    if (!firstCreature) {
        strcat(creatures, ", ");
    }
    strcat(creatures, "\"");
    strcat(creatures, name);
    strcat(creatures, "\"");
    firstCreature = 0;

    // Close JSON array
    strcat(creatures, "]");
}

void removeClosingBracket() {
    int len = strlen(creatures);
    if (len > 0 && creatures[len - 1] == ']') {
        creatures[len - 1] = '\0';  // Truncate
    }
}

//*****************************************************************************
//
//!    main function demonstrates the use of the timers to generate
//! periodic interrupts.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************
int
main(void)
{
    long lRetVal = -1;
    //
    // Initialize board configurations
    BoardInit();

    //
    // Pinmuxing for LEDs
    //
    PinMuxConfig();

    //
    // Enable the SPI module clock
    //
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);

    // Enable clock for UART1
    MAP_PRCMPeripheralClkEnable(PRCM_UARTA1, PRCM_RUN_MODE_CLK);

    // Base address for first timer
    g_ulBase = TIMERA0_BASE;

    // Base address for second timer
    g_ulGPIOBase = TIMERA1_BASE;

    //
    // Configuring the timers
    // (Enables the clock/starts the timer, memory address of the timer, mode that resets itself after timeout, which part of the timer to use (A Half) )
    Timer_IF_Init(PRCM_TIMERA0, g_ulBase, TIMER_CFG_PERIODIC, TIMER_A, 0);

    //
    // Setup the interrupts for the timer timeouts.
    //               (Which timer block, which timer half, function that handles interrupt)
    Timer_IF_IntSetup(g_ulBase, TIMER_A, TimerBaseIntHandler);

    //
    // Turn on the timers feeding values in mSec
    //            (which timer block, which timer half, timer length in us)
    Timer_IF_Start(g_ulBase, TIMER_A, 100);
    //Timer_IF_Start(g_ulGPIOBase, TIMER_A, 100);

    // This configures the GPIO PIN interrupt for both edges
    GPIO_IF_ConfigureNIntEnable(GPIOA0_BASE, 0x40, GPIO_BOTH_EDGES, GPIOA0IntHandler);

    //
    // Initializing the Terminal.
    //
    InitTerm();
    //
    // Clearing the Terminal.
    //
    ClearTerm();

    //
    // I2C Init
    //
    I2C_IF_Open(I2C_MASTER_MODE_FST);

    //
    // Initalizing the UARTA1
    //
    InitUart();

    Message("\t\t****************************************************\n\r");
    Message("\t\t\t                CREATURE CATCH                     \n\r");
    Message("\t\t ****************************************************\n\r");
    Message("\n\n\n\r");


    //
    // Reset the peripheral
    //
    MAP_PRCMPeripheralReset(PRCM_GSPI);


#if MASTER_MODE

    MasterMain();

#else

    SlaveMain();

#endif

    // Adafruit initialize
    Adafruit_Init();

    fillScreen(BLACK);


    //
    // Print Array
    //

    int x = 64 - (CHAR_WIDTH * 2);
    int y = 64 + (CHAR_HEIGHT / 2);

    int xHold = 0;
    //int yHold = 0;

    char buf_str[64];
    int idx = 0;

    int temp_idx = 0;


    // AWS Logic

     // initialize global default app configuration
    g_app_config.host = (signed char*) SERVER_NAME;
    g_app_config.port = GOOGLE_DST_PORT;

    //Connect the CC3200 to the local access point
    UART_PRINT("Connecting [1]... \n\r");

    lRetVal = connectToAccessPoint();


    UART_PRINT("Connected!\n\r");

    UART_PRINT("Setting time...\n\r");
    lRetVal = set_time();
    UART_PRINT("set_time() returned: %d\n\r", lRetVal);
    if(lRetVal < 0) {
        UART_PRINT("Unable to set time in the device\n\r");
        LOOP_FOREVER();
    }

    UART_PRINT("Connecting to TLS endpoint...\n\r");
    lRetVal = tls_connect();
    UART_PRINT("tls_connect() returned: %d\n\r");
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
    }


    createCharacter();

    //
    // Loop forever while the timers run.

    while(FOREVER)
    {
        if (printFlag) {
            if (receiverBuffer[0] != '\0') {
                Report("\n Received!! \n");

                printf("%s \n", receiverBuffer);
                printCharArray64(receiverBuffer);
            }
            printFlag = 0;
        }

        while (finish)
        {
            // Sending Message
            if (buf_char == CHAR_MUTE) {
                fillScreen(BLACK);
                //fillRect(0, 0, 128, 50, BLACK);
                x = 64 - (CHAR_WIDTH * 2);;
                y = 64 + (CHAR_HEIGHT / 2);

                //fillRect(x, y, CHAR_WIDTH, CHAR_HEIGHT, GREEN);
                buf_str[idx] = '\0';

                // put the character in the UART Connection (Sending the Character Data) s
                int k;
                for (k = 0; k < idx; k++) {
                    UARTCharPut(UARTA1_BASE, buf_str[k]);
                }

                UARTCharPut(UARTA1_BASE, '\n');
                Report("\r[Sent]: %s\n", buf_str);

                strncpy(global_buf_copy, buf_str, sizeof(global_buf_copy) - 1);
                global_buf_copy[sizeof(global_buf_copy) - 1] = '\0';  // Ensure null-termination


                // Close because no creatures at the beginning
                strcat(creatures, "]");

                // Send Message Via Email
                http_post(lRetVal);

                http_get(lRetVal);

                idx = 0;
                buf_str[0] = '\0';

                for (k = 0; k < sizeof(buf_str); k++) {
                    buf_str[k] = 0;
                }

                // ENTER MAP LOGIC

                // HIT CREATURE
                int hit = 1;
                char* message;
                int throw_score = 0;

                while(hit){
                    throw_score = creatureCatch();

                    if(throw_score == 1){
                        printf("Bad Throw\n");

                        fillRect(0, 60, 128, 20, BLACK);
                        message = "Bad Throw";
                        printString(message);
                        fillRect(0, 60, 128, 20, BLACK);
                        message = "ESCAPED";
                        printString(message);
                    }
                    else if(throw_score == 2){
                        printf("Good Throw\n");

                        fillRect(0, 60, 128, 20, BLACK);
                        message = "Good Throw";
                        printString(message);
                        fillRect(0, 60, 128, 20, BLACK);
                        message = "Creature Caught";
                        printString(message);
                        removeClosingBracket();
                        addCreature("Charmander");
                        http_post(lRetVal);
                    }
                    else if(throw_score == 3){
                        printf("Great Throw\n");

                        fillRect(0, 60, 128, 20, BLACK);
                        message = "Great Throw";
                        printString(message);
                        fillRect(0, 60, 128, 20, BLACK);
                        message = "Creature Caught";
                        printString(message);
                        removeClosingBracket();
                        addCreature("Pikachu");
                        http_post(lRetVal);
                    }
                }

            }

            // REPEAT LETTER
            else if (pxFlag) {
                if (tap == 1){
//                    if (x == 0 && y > 0){
//                        yHold = y;
//                        yHold -= CHAR_HEIGHT + 1;
//                    }

                    xHold = x;
                    xHold -= CHAR_WIDTH + CHAR_SPACING; // storing letter space before
                    drawChar(xHold, y, 0, BLACK, BLACK, 1); // clear behind letter
                    drawChar(xHold, y, buf_char, WHITE, BLACK, 1); // over write
                    pxFlag = 0;

                    temp_idx = idx - 1;
                    buf_str[temp_idx] = buf_char;
                }
                else {
                    drawChar(xHold, y, 0, BLACK, BLACK, 1);
                    drawChar(xHold, y, buf_char, WHITE, BLACK, 1);
                    pxFlag = 0;

                    buf_str[temp_idx] = buf_char;
                }


            }
            // DELETE
            else if (buf_char == '1') {
                // move back a space
                x -= CHAR_WIDTH + CHAR_SPACING;

                idx -= 1;
                buf_str[idx] = ' ';

                // Case: Passed Boundary
                if (x <= 0){
                    // Case: Y Passed Boundary
                    if (y <= 0) {
                        y = 0;
                        x = 0;
                        // print empty space
                        fillRect(x, y, CHAR_WIDTH, CHAR_HEIGHT, BLACK);
                    }
                    else{
                        // print empty space
                        fillRect(x, y, CHAR_WIDTH, CHAR_HEIGHT, BLACK);
                        x = OLED_WIDTH - 2;
                        y -= CHAR_HEIGHT + 1;
                    }
                }
                else{
                    // print empty space
                    fillRect(x, y, CHAR_WIDTH, CHAR_HEIGHT, BLACK);
                }

                finish = 0;

            }

            // FIRST OR NEW LETTER
            else{
                // draw the character
                drawChar(x, y, buf_char, WHITE, BLACK, 1);

                buf_str[idx] = buf_char;
                idx++;

                // move to next space
                x += CHAR_WIDTH + CHAR_SPACING;

                if (x + CHAR_WIDTH > OLED_WIDTH) {
                    x = 0;
                    y += CHAR_HEIGHT + 1;
                }

                if (y + CHAR_HEIGHT > OLED_HEIGHT) {
                    fillScreen(RED);
                    char* error = "ERROR: Message too long";
                    printString(error);
                }

            }

            finish = 0;
        }


    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

static int http_post(int iTLSSockID){
    // stores the full HTTP Request
    char acSendBuff[512];

    // stores the server's response
    char acRecvbuff[1460];

    // stores the number (as string) for the Content-Length Header
    char cCLLength[200];

    char* pcBufHeaders;
    int lRetVal = 0;


    // Example usage
//    addCreature("Pikachu");
//    addCreature("Charmander");

    // Close JSON array
    // strcat(creatures, "]");


    // Final payload
    char jsonPayload[256];
    snprintf(jsonPayload, sizeof(jsonPayload),
        "{"
            "\"state\": {"
                "\"desired\": {"
                    "\"var\": {"
                        "\"%s\": %s"
                    "}"
                "}"
            "}"
        "}", global_buf_copy, creatures);

    // Update JSON Payload
//    char jsonPayload[256];
//    snprintf(jsonPayload, sizeof(jsonPayload),
//        "{"
//            "\"state\": {"
//                "\"desired\": {"
//                    "\"var\": \"%s\""
//                "}"
//            "}"
//        "}", global_buf_copy);

    // fills buffer with HTTP Request Line and Headers
    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, POSTHEADER);
    pcBufHeaders += strlen(POSTHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    // Sending Data
    int dataLength = strlen(jsonPayload);

    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);
    sprintf(cCLLength, "%d", dataLength);

    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);

    // Append the JSON Payload
    strcpy(pcBufHeaders, jsonPayload);
    pcBufHeaders += strlen(jsonPayload);

    int testDataLength = strlen(pcBufHeaders);

    UART_PRINT(acSendBuff);


    //
    // Send the packet to the server */
    //

    // Send POST Request
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);

    // Notify Failure
    if(lRetVal < 0) {
        UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    // Receive Server's Response
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);

    // Receiver Response Failed
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }

    // If Successful, Null-Terminate and Print
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }

    return 0;
}

static int http_get(int iTLSSockID) {
     // stores the full HTTP Request
    char acSendBuff[512];

    // stores the server's response
    char acRecvbuff[1460];


    char* pcBufHeaders;
    int lRetVal = 0;

    // fills buffer with HTTP Request Line and Headers
    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, GETHEADER);
    pcBufHeaders += strlen(GETHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");


    UART_PRINT(acSendBuff);


    //
    // Send the packet to the server */
    //

    // Send GET Request
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);

    // Notify Failure
    if(lRetVal < 0) {
        UART_PRINT("GET failed. Error Number: %i\n\r", lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    // Receive Server's Response
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);

    // Receiver Response Failed
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }

    // If Successful, Null-Terminate and Print
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT("RECEIVED BUFFER to parse: \n");
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }

    return 0;

}
