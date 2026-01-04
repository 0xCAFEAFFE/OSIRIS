/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Public interface for power related functions
===============================================================================
*/

#include "pwr.h"
//---------------
#include "gpio.h"
#include "keys.h"
#include "lcd.h"
#include "rad.h"
#include "rtc.h"
#include "uart.h"

typedef enum
{
	PWR_SRC_BAT = 0,
	PWR_SRC_USB = 1
} PWR_Src_t;

// internal variables
static bool usbChangedFlag;
static volatile PWR_Src_t pwrSrc;

// init USB connect / disconnect detection
void PWR_Init(void)
{
	// determine power source - enable UART only if USB connected
	pwrSrc = GPIO_GetPin(PIN_VUSB);
	UART_Enable(pwrSrc);
	
	// clear & enable INT0 (both edges on PIN_VUSB)
	SET(EICRA, ISC00);
	CLR_FLAG(EIFR, INTF0);
	SET(EIMSK, INT0);
}

// disable UART and USB interrupt
void PWR_DeInit(void)
{
	UART_Enable(false);
	CLR(EIMSK, INT0);
}

// this returns true once if USB state changed
bool PWR_CheckUsbEvent(void)
{
	if (usbChangedFlag)
	{
		usbChangedFlag = false;
		return true;
	}
	
	return false;
}

// force system reset
void PWR_Reset(void)
{
	// global interrupt disable
	cli();
	
	// enable watchdog with shortest timeout value
	wdt_reset();
	wdt_enable(WDTO_15MS);
	
	// let watchdog time out
	while (true);
}

// go to PowerSave mode, keeps async T2 running
void PWR_SleepMode(void)
{
	cli();				// global interrupts disable
	set_sleep_mode(SLEEP_MODE_PWR_SAVE);
	sleep_enable();		// set SE bit
	sei();				// global interrupts re-enable
	sleep_cpu();		// go to power save mode
	// **** CPU sleeps here until woken by interrupt ****
	sleep_disable();	// clear SE bit
}

// power off
void PWR_Shutdown(void)
{
	// wait until shutdown message sent
	UART_Printf("Shutdown..");
	while (UART_TxBusy());

	// disable LCD
	LCD_Clear();
	LCD_Enable(false);

	// disable beeper & soft-latch
	GPIO_SetPin(PIN_BEEP_EN, false);
	GPIO_SetPin(PIN_CLICK_EN, false);
	GPIO_SetPin(PIN_VREG_EN, false);

	// disable watchdog
	wdt_reset();
	wdt_disable();
	
	if (pwrSrc == PWR_SRC_BAT)
	{
		// disable interrupts globally
		cli();
			
		// wait for power to run out
		while (true);
	}
	else // if (pwrSrc == PWR_SRC_USB)
	{
		// disable all interrupts except keys
		PWR_DeInit();
		RAD_DeInit();
		RTC_DeInit();
		KEYS_DeInit();
		
		// wait for red key release
		while (!GPIO_GetPin(PIN_KEY_RED));
		_delay_ms(100); // debounce, just in case

		// go to sleep
		PWR_SleepMode();
		
		// if woken up by keys -> reset to emulate power-on
		PWR_Reset();
	}
}

// INT0 external interrupt ISR; USB dis/connect
ISR(INT0_vect)
{
	bool usb = GPIO_GetPin(PIN_VUSB);
	if (usb != pwrSrc)
	{
		UART_Enable(usb);	// UART not needed if USB not connected
		pwrSrc = usb;		// set new power source
		usbChangedFlag = true;
		return;
	}
}

