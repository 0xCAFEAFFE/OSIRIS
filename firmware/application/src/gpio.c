/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Implementation of readable GPIO functions
===============================================================================
*/

#include "gpio.h"

// internal function prototypes
static void SetBit(sfr *target_byte, byte target_bit, bool state);
static void SetPortBit(byte pin, bool state);

// initialize all GPIO pins
void GPIO_Init(void)
{
	for (byte i=0; i<PIN_NUM; i++)
	{
		// set pin direction
		GPIO_ConfigPin(i, gpio[i].dir);
		
		// for outputs: set output to default state
		// for inputs: enable/disable pull-ups
		if (gpio[i].dir==IN) { GPIO_PullupPin(i, gpio[i].init); }
		else { GPIO_SetPin(i, gpio[i].init); }
	}
}

// configure pin as input or output
void GPIO_ConfigPin(byte pin, bool dir)
{
	if (gpio[pin].port==PB)		{ SetBit(&DDRB, gpio[pin].n, dir); }
	else if (gpio[pin].port==PC) { SetBit(&DDRC, gpio[pin].n, dir); }
	else if (gpio[pin].port==PD) { SetBit(&DDRD, gpio[pin].n, dir); }
	else if (gpio[pin].port==PE) { SetBit(&DDRE, gpio[pin].n, dir); }
	else { SYS_EXCEPTION(); }
}

// set output pin state
void GPIO_SetPin(byte pin, bool state)
{
	SYS_Assert(gpio[pin].dir==OUT);
	SetPortBit(pin, state);
}

// get input pin state
bool GPIO_GetPin(byte pin)
{
	SYS_Assert(gpio[pin].dir==IN);
	
	if (gpio[pin].port==PB)		 { return (bool)GET(PINB, gpio[pin].n); }
	else if (gpio[pin].port==PC) { return (bool)GET(PINC, gpio[pin].n); }
	else if (gpio[pin].port==PD) { return (bool)GET(PIND, gpio[pin].n); }
	else if (gpio[pin].port==PE) { return (bool)GET(PINE, gpio[pin].n); }
	else
	{
		SYS_EXCEPTION();
		return false;
	}
}

// enable or disable pull-up resistor on input pin
void GPIO_PullupPin(byte pin, bool enable)
{
	SYS_Assert(gpio[pin].dir==IN);
	SetPortBit(pin, enable);
}

// toggle output pin
void GPIO_TogglePin(byte pin)
{
	SYS_Assert(gpio[pin].dir==OUT);
	
	if (gpio[pin].port == PB)		{ INV(PORTB, gpio[pin].n); }
	else if (gpio[pin].port == PC)	{ INV(PORTC, gpio[pin].n); }
	else if (gpio[pin].port == PD)	{ INV(PORTD, gpio[pin].n); }
	else if (gpio[pin].port == PE)	{ INV(PORTE, gpio[pin].n); }
	else { SYS_EXCEPTION(); }
}

// function to set or clear a single SFR bit
static void SetBit(sfr *target_byte, byte target_bit, bool state)
{
	if (state) { SET(*target_byte, target_bit); }
	else { CLR(*target_byte, target_bit); }
}

// set bit of port register, wrap it with assert for safety
static void SetPortBit(byte pin, bool state)
{
	if (gpio[pin].port==PB)		 { SetBit(&PORTB, gpio[pin].n, state); }
	else if (gpio[pin].port==PC) { SetBit(&PORTC, gpio[pin].n, state); }
	else if (gpio[pin].port==PD) { SetBit(&PORTD, gpio[pin].n, state); }
	else if (gpio[pin].port==PE) { SetBit(&PORTE, gpio[pin].n, state); }
	else { SYS_EXCEPTION(); }
}

// -------------------------------------- EOF --------------------------------------