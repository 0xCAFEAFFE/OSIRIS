/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Public interface for GPIO functions
===============================================================================
*/

#ifndef GPIO_H_
#define GPIO_H_

#include "sys.h"

// GPIO pin property struct
typedef struct
{
	// 7 bits total
	byte port	: 2;	// register block	(B/C/D/E)
	byte n		: 3;	// pin number		(0..7)
	bool dir	: 1;	// direction		(0=in/1=out)
	bool init	: 1;	// default state	(0=lo/1=hi)
} GPIO_PinStruct_t;

// GPIO port enum
typedef enum
{
	PB = 0u,
	PC = 1u,
	PD = 2u,
	PE = 3u
} GPIO_Port_t;

// pin names
typedef enum
{
	PIN_RAD_IMP		= 0u,
	PIN_VREG_EN		= 1u,
	PIN_RFU1		= 2u, // reserved for future use
	PIN_RFU2		= 3u,
	PIN_KEY_GRN		= 4u,
	PIN_KEY_YEL		= 5u,
	PIN_KEY_RED		= 6u,
	PIN_RFU3		= 7u,
	PIN_HV_EN		= 8u,
	PIN_BEEP_EN		= 9u,
	PIN_SPI_MOSI	= 10u,
	PIN_SPI_MISO	= 11u,
	PIN_SPI_SCK		= 12u,
	PIN_RFU4		= 13u,
	PIN_CLICK_EN	= 14u,
	PIN_BAT_STAT	= 15u,
	PIN_LCD_RS		= 16u,
	PIN_LCD_CS		= 17u,
	PIN_HV_GATE		= 18u,
	PIN_RFU5		= 19u,
	PIN_UART_RX		= 20u,
	PIN_UART_TX		= 21u,
	PIN_VUSB		= 22u,
	PIN_NUM			= 23u // needs to be the last one
} GPIO_Pin_t;

// pin name mapping
static const __flash GPIO_PinStruct_t gpio[PIN_NUM] =
{
	// port, pin, dir, init
	{PD,3u,IN,LO},	// PD3 - IMP_INT
	{PD,4u,OUT,HI}, // PD4 - PWR_EN
	{PE,0u,IN,HI},	// PE0 - [unused]
	{PE,1u,IN,HI},	// PE1 - [unused]
	{PD,5u,IN,LO},	// PD5 - KEY_GRN
	{PD,6u,IN,LO},	// PD6 - KEY_YEL
	{PD,7u,IN,LO},	// PD7 - KEY_RED
	{PB,0u,IN,HI},	// PB0 - [unused]
	{PB,1u,OUT,LO}, // PB1 - HV_EN
	{PB,2u,OUT,LO}, // PB2 - IMP_BEEP_EN
	{PB,3u,OUT,HI}, // PB3 - LCD_SI
	{PB,4u,IN,LO},	// PB4 - MISO
	{PB,5u,OUT,HI}, // PB5 - LCD_CLK
	{PE,2u,IN,HI},	// PE2 - [unused]
	{PC,0u,OUT,LO}, // PC0 - IMP_CLICK_EN
	{PC,1u,IN,LO},  // PC1 - BAT_STAT
	{PC,2u,OUT,HI}, // PC2 - LCD_RS
	{PC,3u,OUT,HI}, // PC3 - LCD_CS
	{PC,4u,IN,LO},	// PC4 - HV_MON
	{PC,5u,IN,HI},	// PC5 - [unused]
	{PD,0u,IN,LO},	// PD0 - RX
	{PD,1u,OUT,HI}, // PD1 - TX
	{PD,2u,IN,LO},	// PD2 - VUSB
};

// public function declarations
void GPIO_Init(void);
void GPIO_ConfigPin(byte gpiopin, bool state);
void GPIO_SetPin(byte gpiopin, bool state);
bool GPIO_GetPin(byte gpiopin);
void GPIO_PullupPin(byte gpiopin, bool enable);
void GPIO_TogglePin(byte gpiopin);

#endif /* GPIO_H_ */