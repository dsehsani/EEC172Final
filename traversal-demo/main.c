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
#include "timer_if.h"
#include "gpio_if.h"

// Added for GPIO Interrupt
#include "gpio.h"

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


// map interace
#include "map_bitmap.h"
#include "tiles.h"


#define CHAR_WIDTH     5
#define CHAR_HEIGHT    7
#define CHAR_SPACING   1
#define OLED_WIDTH     128
#define OLED_HEIGHT    128



#define MIN     (4 + 1)
#define MAX     (OLED_HEIGHT - 4 - 1)


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

//*****************************************************************************
//                      Global Variables for Functionality
//*****************************************************************************
// long long is 64 bits long and the input signal is 48 bits + 1 bit at the end
static uint64_t bitStream = 0;
//static uint64_t bitMask = (1ULL << 49);
static volatile unsigned long startTime = 0;
// maybe change startTime to a unsigned long like the counter


//*****************************************************************************
//                      Design
//*****************************************************************************



//*****************************************************************************
//
// Globals used by the timer interrupt handler.
//
//*****************************************************************************
static volatile unsigned long g_ulSysTickValue;
static volatile unsigned long g_ulBase;
static volatile unsigned long g_ulGPIOBase;
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


//*****************************************************************************
//
// Global Hash Table for Unique ID For Remote Control
//
//*****************************************************************************
// typedef struct {
//     uint64_t key;
//     const char* value;
// }remoteEntry;

// const remoteEntry remoteMap[] = {
//     {0xA00200804C4C, "0"},
//     {0xA00200800404, "1"},
//     {0xA00200804444, "2"},
//     {0xA00200802424, "3"},
//     {0xA00200806464, "4"},
//     {0xA00200801414, "5"},
//     {0xA00200805454, "6"},
//     {0xA00200803434, "7"},
//     {0xA00200807474, "8"},
//     {0xA00200800C0C, "9"},
//     {0xA00200802626, "MUTE"},
//     {0xA00200807676, "LAST"},
// };


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
//!       Displays "Hello World!"
//
//*****************************************************************************
void printString(char* message) {
    //fillScreen(BLACK);
    int x = 0;
    int y = 0;
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
//! The interrupt handler for the second timer interrupt.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************
//void
//TimerRefIntHandler(void)
//{
//    //
//    // Clear the timer interrupt.
//    //
//    Timer_IF_InterruptClear(g_ulRefBase);
//
//    g_ulRefTimerInts ++;
//    GPIO_IF_LedToggle(MCU_RED_LED_GPIO);
//}

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

                finish = 1;

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
//! Main Function: Visualizes Character Movement
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************
#define TILE_SIZE 16

#define MAP_WIDTH 32
#define MAP_HEIGHT 32

extern const uint8_t map_bitmap[MAP_HEIGHT][MAP_WIDTH];
extern const uint16_t* tile_set[NUM_TILE_TYPES];

void drawTile(int x0, int y0, const uint16_t* tile) {
    int y;
    int x;


    for (y = 0; y < TILE_SIZE; y++) {
        for (x = 0; x < TILE_SIZE; x++) {
            drawPixel(x0 + x, y0 + y, tile[y * TILE_SIZE + x]);
        }
    }
}



void drawMap(int map_width, int map_height) {
    int row;
    int col;

    for (row = 0; row < map_height; ++row) {
        for (col = 0; col < map_width; ++col) {
            int tile_index = map_bitmap[row][col];
            drawTile(col * TILE_SIZE, row * TILE_SIZE, tile_set[tile_index]);
        }
    }
}


void drawPlayer(int tile_x, int tile_y)  {  
    drawTile(tile_x * TILE_SIZE, tile_y * TILE_SIZE, tile_set[5]);
}

void erasePlayer(int tile_x, int tile_y) {
    if (tile_x < 0 || tile_x >= MAP_WIDTH || tile_y < 0 || tile_y >= MAP_HEIGHT) {
        return;
    }

    int tile_index = map_bitmap[tile_y][tile_x];
    if (tile_index >= NUM_TILE_TYPES) return;

    drawTile(tile_x * TILE_SIZE, tile_y * TILE_SIZE, tile_set[tile_index]);
}


main(void)
{
    BoardInit();
    PinMuxConfig();

    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_UARTA1, PRCM_RUN_MODE_CLK);

    g_ulBase = TIMERA0_BASE;
    g_ulGPIOBase = TIMERA1_BASE;

    Timer_IF_Init(PRCM_TIMERA0, g_ulBase, TIMER_CFG_PERIODIC, TIMER_A, 0);
    Timer_IF_IntSetup(g_ulBase, TIMER_A, TimerBaseIntHandler);
    Timer_IF_Start(g_ulBase, TIMER_A, 100);

    GPIO_IF_ConfigureNIntEnable(GPIOA0_BASE, 0x40, GPIO_BOTH_EDGES, GPIOA0IntHandler);

    InitTerm();
    ClearTerm();

    InitUart();

    Message("\t\t****************************************************\n\r");
    Message("\t\t\t        CC3200 Signal Decoding        \n\r");
    Message("\t\t This program will receive input signal from the remote,  \n\r");
    Message("\t\t process them, and display the number pressed. \n\r") ;
    Message("\t\t ****************************************************\n\r");
    Message("\n\n\n\r");


    MAP_PRCMPeripheralReset(PRCM_GSPI);

    #if MASTER_MODE

        MasterMain();

    #else

        SlaveMain();

    #endif


    Adafruit_Init();
    fillScreen(BLACK);


    

    drawMap(MAP_WIDTH, MAP_HEIGHT);

    // Character Positioning
    int player_x = 4;
    int player_y = 4;


    drawPlayer(player_x, player_y);

    while(FOREVER) {
        while (finish) {
            if (buf_char == CHAR_2) {
                // UP Logic
                erasePlayer(player_x, player_y);
                player_y += 1;
                drawPlayer(player_x, player_y);

            }

            if (buf_char == CHAR_4) {
                // LEFT Logic
                erasePlayer(player_x, player_y);
                player_x += 1;
                drawPlayer(player_x, player_y);
            }

            if (buf_char == CHAR_6) {
                // Right Logic
                erasePlayer(player_x, player_y);
                player_x -= 1;
                drawPlayer(player_x, player_y);
            }

            if (buf_char == CHAR_8) {
                 // Down Logic
                erasePlayer(player_x, player_y);
                player_y -= 1;
                drawPlayer(player_x, player_y);
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

