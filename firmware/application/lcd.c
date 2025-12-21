/*
 * Copyright (c) 2014 by ELECTRONIC ASSEMBLY <technik@lcd-module.de>
 * EA DOGM-Text (ST7036) library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 *
 * Modified beyond recognition by Nicolai Sawilla for Project OSIRIS in 2022
 */

#include "lcd.h"
//---------------
#include "gpio.h"

// internal defines
#define LCD_LINES			2u
#define LCD_COLUMNS			16u
#define LCD_INIT_LEN		8u
#define LCD_CONTRAST		35u
#define LCD_SPI_DELAY		30u // data commands need 26us

// pin mapping
#define LCD_PIN_CS		PIN_LCD_CS
#define LCD_PIN_SDI		PIN_SPI_MOSI
#define LCD_PIN_CLK		PIN_SPI_SCK
#define LCD_PIN_RST		PIN_LCD_RS

// internal variables
static const uint8_t init_DOGM162_5V[LCD_INIT_LEN] = {0x39, 0x1C, 0x52, 0x69, 0x74, 0x38, 0x01, 0x06};
static uint8_t cmdByte;
static byte currentLine;

// max of 8 custom chars can be defined here
// BASCOM-AVR was used to determine these values
static const __flash uint8_t lcdCustomChars[LCD_NUM_SYMBOLS][8]=
{
	{10,31,17,17,17,17,17,31},	// battery empty - 0/5 bars - frame only
	{10,31,17,17,17,17,31,31},	// 1/5
	{10,31,17,17,17,31,31,31},	// 2/5
	{10,31,17,17,31,31,31,31},	// 3/5
	{10,31,17,31,31,31,31,31},	// 4/5
	{10,31,31,31,31,31,31,31},	// battery full - 5/5 bars
	{2,4,8,31,2,4,8,32},		// USB connected
	{7,4,7,4,4,31,17,31}		// keys locked
};

// internal function prototypes
static void SpiOut(uint8_t dat);
static void SpiInit(void);
static void SpiPutChar(uint8_t dat);
static void CursorEn(bool on);
static void SendCommand(uint8_t dat);
static void SendData(uint8_t dat);
static void SetContrast(uint8_t contr);

//---------------------------------------------------- Public Functions ----------------------------------------------------

/*----------------------------
Func: DOG-INIT
Desc: Initializes SPI Hardware/Software and DOG Displays
------------------------------*/
void LCD_Init(void)
{
	cmdByte = 0x0C; // Display on/off control status at power on reset, needed for cursor on/off and Display on/off
	
	SpiInit();

	// Init DOGM-Text displays, depending on users choice of supply voltages and lines
	uint8_t *ptr_init;
	ptr_init = (uint8_t*)init_DOGM162_5V;
	
	GPIO_SetPin(LCD_PIN_RST, LO);
	for (uint8_t i=0; i < LCD_INIT_LEN; i++) { SendCommand(*ptr_init++); }
	
	// define custom symbols
	for (byte i=0; i<LCD_NUM_SYMBOLS; i++)
	{
		SendCommand(0x40+8*i);

		for (uint8_t j=0; j<8; j++)
		{
			volatile byte data = lcdCustomChars[i][j]; // avoid optimization shenanigans with __flash
			SendData(data);
		}
	}
	
	// clear & reset position
	LCD_Clear();
	CursorEn(false);
	SetContrast(LCD_CONTRAST);
	LCD_Enable(true);
}

/*----------------------------
Func: String
Desc: Shows a String on the DOG-Display
Vars: String
------------------------------*/
void LCD_Printf(uint8_t line, const char *formatstr, ...)
{
	// select line
	if (line) { LCD_Position(line, 0); }
	
	// variadic sorcery
	va_list args;
	va_start(args, formatstr);
	char buffer[LCD_COLUMNS+1] = {0};
	vsnprintf(buffer, LCD_COLUMNS+1, formatstr, args);
	va_end(args);

	// send data to LCD
	char *str = buffer;
	while (*str) { SendData(*str++); }
}

/*----------------------------
Func: ascii
Desc: Shows a Character on the DOG-Display
Vars: Character
------------------------------*/
void LCD_PrintChar(uint8_t line, uint8_t column, char character)
{
	LCD_Position(line, column);
	SendData(character);
}

/*----------------------------
Func: position
Desc: Sets a new cursor position DOG-Display
Vars: column (1..16), line (1..3)
------------------------------*/
void LCD_Position(uint8_t line, uint8_t column)
{
	uint8_t cmd = 0;
	if (column == 0) { column = 1; }		// minimum column 1
	if (column > 16) { column = 16; }	// maximum column 16
	
	// 2-Line display second line address
	if (line == 2) { cmd = 0x40; }
	
	SendCommand(0x80 + cmd + column - 1); // DOG display starts with column 0 --> decrement
}

/*----------------------------
Func: displ_onoff
Desc: turns the entire DOG-Display on or off
Vars: on (true = display on, false = display off)
------------------------------*/
void LCD_Enable(bool on) 
{
	if (on) {cmdByte |= 0x04;}
	else {cmdByte &= ~0x04;}

	SendCommand(cmdByte);
}

/*----------------------------
Func: clear_display
Desc: clears the entire DOG-Display
Vars: ---
------------------------------*/
void LCD_Clear(void) 
{
	SendCommand(0x01); // clear display and return home
	currentLine = 1;
}

/*----------------------------
Func: cursor_onoff
Desc: turns the cursor on or off
Vars: on (true = cursor blinking, false = cursor off)
------------------------------*/
static void CursorEn(bool on)
{
	if (on) {cmdByte |= 0x01;}
	else {cmdByte &= ~0x01;}

	SendCommand(cmdByte);
}

/*----------------------------
Func: contrast
Desc: sets contrast to the DOG-Display
Vars: uint8_t contrast (0..63)
------------------------------*/
static void SetContrast(uint8_t contr) 
{
	contr &=0x3F; // contrast has only 6 bits
	
	// switch to instruction table 1 depending on display lines
	SendCommand(0x39);
	SendCommand(0x50 | (contr >> 4));	// booster off, 2 high bits of contrast
	SendCommand(0x70 | (contr&0x0F));   // 4 low bits of contrast
	
	// switch to instruction table 0 depending on display lines
	SendCommand(0x38);
}

//---------------------------------------------------- Internal Functions ----------------------------------------------------

/*----------------------------
Func: command
Desc: Sends a command to the DOG-Display
Vars: data
------------------------------*/
static void SendCommand(uint8_t dat) 
{
	GPIO_SetPin(LCD_PIN_RST, LO);
	SpiPutChar(dat);

	// extra delay for return home or clear display cmds
	// 1.08ms according to datasheet 
	if (dat <= 0x03) { _delay_us(1080); }
}

/*----------------------------
Func: data
Desc: Sends data to the DOG-Display
Vars: data
------------------------------*/
static void SendData(uint8_t dat) 
{
	GPIO_SetPin(LCD_PIN_RST, HI);
	SpiPutChar(dat);
}

/*----------------------------
Func: SpiInit
Desc: Initializes SPI Hardware/Software
Vars: CS-Pin, MOSI-Pin, SCK-Pin (MOSI=SCK Hardware else Software)
------------------------------*/
static void SpiInit(void) 
{
	// Set CS to deselect slaves
	GPIO_ConfigPin(LCD_PIN_CS, OUT);
	GPIO_SetPin(LCD_PIN_CS, HI);
	
	// Set Data pin as output
	GPIO_ConfigPin(LCD_PIN_SDI, OUT);

	// Set SPI-Mode 3: CLK idle high, rising edge, MSB first
	GPIO_ConfigPin(LCD_PIN_CLK, OUT);
	GPIO_SetPin(LCD_PIN_CLK, HI);
}

/*----------------------------
Func: SpiPutChar
Desc: Sends one uint8_t using CS
Vars: data
------------------------------*/
static void SpiPutChar(uint8_t dat) 
{
	GPIO_SetPin(LCD_PIN_CS, LO);
	SpiOut(dat);
	GPIO_SetPin(LCD_PIN_CS, HI);
	_delay_us(LCD_SPI_DELAY);
}

/*----------------------------
Func: SpiOut
Desc: Sends one uint8_t, no CS
Vars: data
------------------------------*/
static void SpiOut(uint8_t dat) 
{
	uint8_t i=8;
	do
	{
		GPIO_SetPin(LCD_PIN_SDI, (dat & 0x80));
		GPIO_SetPin(LCD_PIN_CLK, LO);
		dat <<= 1;
		GPIO_SetPin(LCD_PIN_CLK, HI);
	} while (--i);
}

// -------------------------------------- EOF --------------------------------------
