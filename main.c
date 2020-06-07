#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_uart.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/debug.h"
#include "inc/tm4c1230h6pm.h"


//*****************************************************************************
//
// Macros used in this application.
//
//*****************************************************************************
#define NUM_UART_DATA    4
#define KEYPAD_ROW GPIO_PORTA_BASE
#define KEYPAD_COL GPIO_PORTB_BASE

//*****************************************************************************
//
// Global Variables used in this application.
//
//*****************************************************************************

unsigned char room_input=0;
char room_pwd[4];
unsigned char room_number;
unsigned char input;
unsigned char i=0,j=0,x=0,y=0,k=0;

/* Room Database*/
struct room_data
{
    char room_no;
    char room_status[10];
    char room_password[4];

};


//*****************************************************************************
//
// Function used in this application.
//
//*****************************************************************************

void delayMs(int n);
void delayUs(int n);
void keypad_init(void);
unsigned char keypad_kbhit(void);
void keypad_init(void);
unsigned char get_key(void);
void open_door(void);
void close_door(void);
void Solenoid_Init(void);
char compareArray(char a[],char b[],int size);
void UARTSend(uint32_t ui32UARTBase, const uint8_t *pui8Buffer, uint32_t ui32Count);

                                                /*Main Function*/
int main(void)
{

    unsigned char key;

    struct room_data room_in_hotel[10];


        /*Set the clock*/
    MAP_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);


        /*Enable Peripheral clock*/
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);


    /*Set GPIO A0 and A1 as UART pins*/

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    /*Set UART Configuration*/
    MAP_UARTConfigSetExpClk(UART0_BASE, MAP_SysCtlClockGet(), 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    /*Initialization OF Keypad*/
    keypad_init();

    /*Initialization OF Solenoid*/
    Solenoid_Init();

    /*Print Hotel Info*/


    UARTSend(UART0_BASE, (uint8_t *)"\n Welcome to Hotel Database ->",
             strlen("\n Welcome to Hotel Database ->"));

    UARTSend(UART0_BASE, (uint8_t *)"Press \"1\" to check-in\n ->",
                 strlen("Press \"1\" to check-in\n ->"));

    UARTSend(UART0_BASE, (uint8_t *)"Press \"2\" to check-out\n ->",
                     strlen("Press \"2\" to check-out\n ->"));


    while(1)
    {

        key = get_key(); /* read the keypad */

        room_input=UARTCharGet(UART0_BASE); /* read the UART */

        if (key != 0)
        {
            /* if a key is pressed */

           for(x=0;x<=3;x++)
           {
               room_pwd[x]=key;
           }

           for(y=0;y<=9;y++)
           {
               if(compareArray(room_pwd,room_in_hotel[y].room_status,4))
               {
                   open_door();
               }
               else
               {
                   close_door();
               }
           }
        }

            if(room_input=='1')
            {
                UARTSend(UART0_BASE, (uint8_t *)"Enter Room NO",
                                 strlen("Enter Room NO"));
                input =UARTCharGet(UART0_BASE);

                if(input<=10)
                {
                          if(room_in_hotel[input].room_status=="Occupied")
                          {
                              UARTSend(UART0_BASE, (uint8_t *)"Room is Occupied",
                                                               strlen("Room is Occupied"));
                          }
                          else
                          {
                              UARTSend(UART0_BASE, (uint8_t *)"Enter Room status",
                                                              strlen("Enter Room status"));
                              for(j=0;j<='\n';j++)
                                 {

                                  room_in_hotel[input].room_status[j]=UARTCharGet(UART0_BASE);
                                 }
                              UARTSend(UART0_BASE, (uint8_t *)"Enter Room Password",
                                                      strlen("Enter Room Password"));
                                 for(j=0;j<=3;j++)
                                  {
                                   room_in_hotel[input].room_password[j]=UARTCharGet(UART0_BASE);
                                  }


                          }

                }
                else
                {
                    UARTSend(UART0_BASE, (uint8_t *)"Entered wrong input",
                                                     strlen("No room Available"));
                }




            }

            if(room_input=='2')
            {
                UARTSend(UART0_BASE, (uint8_t *)"Enter Room NO to check out",
                                                 strlen("Enter Room NO"));
                input =UARTCharGet(UART0_BASE);

                if(input<=10)
                {
                    strcpy(room_in_hotel[input].room_status,"Free");
                    strcpy(room_in_hotel[input].room_password,"0000");
                }

            }
    }





}


/* this function initializes the ports connected to the keypad */
void keypad_init(void)
{
SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

GPIO_PORTF_DIR_R |= 0x0F; /* set row pins 3-0 as output */
GPIO_PORTF_DEN_R |= 0x0F; /* set row pins 3-0 as digital pins */
GPIO_PORTF_ODR_R |= 0x0F; /* set row pins 3-0 as open drain */

GPIO_PORTE_DIR_R &= ~0xF0; /* set column pin 7-4 as input */
GPIO_PORTE_DEN_R |= 0xF0; /* set column pin 7-4 as digital pins */
GPIO_PORTE_ODR_R |= 0xF0; /* enable pull-ups for pin 7-4 */
}

/* This is a non-blocking function to read the keypad. */
/* If a key is pressed, it returns the key label in ASCII encoding. Otherwise, it returns a 0 (not ASCII 0). */
unsigned char get_key(void)
{

const unsigned char keymap[4][4] = {
{ '1', '2', '3', 'A'},
{ '4', '5', '6', 'B'},
{ '7', '8', '9', 'C'},
{ '*', '0', '#', 'D'},
};

int row, col;

/* check to see any key pressed first */
GPIO_PORTF_DATA_R = 0;
col = GPIO_PORTE_DATA_R & 0xF0;
if (col == 0xF0) return 0;
/* no key pressed */

/* If a key is pressed, it gets here to find out which key. */
/* Although it is written as an infinite loop, it will take one of the breaks or return in one pass.*/
while (1)
{

row = 0;
GPIO_PORTF_DATA_R = 0x0E; /* enable row 0 */
delayUs(2); /* wait for signal to settle */
col = GPIO_PORTE_DATA_R & 0xF0;
if (col != 0xF0) break;

row = 1;
GPIO_PORTF_DATA_R = 0x0D; /* enable row 1 */
delayUs(2); /* wait for signal to settle */
col = GPIO_PORTE_DATA_R & 0xF0;
if (col != 0xF0) break;

row = 2;
GPIO_PORTF_DATA_R = 0x0B; /* enable row 2 */
delayUs(2); /* wait for signal to settle */
col = GPIO_PORTE_DATA_R & 0xF0;
if (col != 0xF0) break;

row = 3;
GPIO_PORTF_DATA_R = 0x07; /* enable row 3 */
delayUs(2); /* wait for signal to settle */
col = GPIO_PORTE_DATA_R & 0xF0;
if (col != 0xF0) break;

return 0; /* if no key is pressed */
}

/* gets here when one of the rows has key pressed */
if (col == 0xE0) return keymap[row][0]; /* key in column 0 */
if (col == 0xD0) return keymap[row][1]; /* key in column 1 */
if (col == 0xB0) return keymap[row][2]; /* key in column 2 */
if (col == 0x70) return keymap[row][3]; /* key in column 3 */
return 0; /* just to be safe */
}
unsigned char keypad_kbhit(void)
{
int col;

/* check to see any key pressed */
GPIO_PORTF_DATA_R = 0; /* enable all rows */
col = GPIO_PORTE_DATA_R & 0xF0; /* read all columns */
if (col == 0xF0)
return 0; /* no key pressed */
else
return 1; /* a key is pressed */
}

/* delay n milliseconds (16 MHz CPU clock) */
void delayMs(int n)
{
int i, j;
for(i = 0 ; i < n; i++)
for(j = 0; j < 3180; j++)
{} /* do nothing for 1 ms */
}

/* delay n microseconds (16 MHz CPU clock) */
void delayUs(int n)
{
int i, j;
for(i = 0 ; i < n; i++)
for(j = 0; j < 3; j++)
{} /* do nothing for 1 us */
}
        /*Function to Open Door*/
void open_door(void)
{
    GPIO_PORTE_DATA_R|=(1<<0); //Open door
}
        /*Function to Close Door*/
void close_door(void)
{
    GPIO_PORTE_DATA_R&=~(1<<0); //Close door
}
        /*Function to Init Solenoid*/
void Solenoid_Init(void)
{
    GPIO_PORTC_DIR_R |= (1<<0); /* set PIN 0 as output */
    GPIO_PORTC_DEN_R |= (1<<0); /*Digital Pin*/


}
        /*Function to Compare Array*/
char compareArray(char a[],char b[],int size) {
    int i;
    for(i=0;i<size;i++){
        if(a[i]!=b[i])
            return 1;
    }
    return 0;
}
        /*Function to send Uart Data*/
void UARTSend(uint32_t ui32UARTBase, const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    //
    // Loop while there are more characters to send.
    //
    while(ui32Count--)
    {
        //
        // Write the next character to the UART.
        //
        MAP_UARTCharPut(ui32UARTBase, *pui8Buffer++);
    }
}
