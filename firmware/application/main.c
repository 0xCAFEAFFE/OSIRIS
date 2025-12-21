/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Main application source
===============================================================================
*/

#include "adc.h"
#include "cmd.h"
#include "gpio.h"
#include "keys.h"
#include "lcd.h"
#include "pwr.h"
#include "rad.h"
#include "rtc.h"
#include "sys.h"
#include "uart.h"
#include "ui.h"

// internal function prototypes
static bool InitSystem(void);

// application boot vector
int main(void)
{
	// initialize all subsystems
	bool ok = InitSystem();
	SYS_Assert(ok);

	// ================ main loop ================
	// this block will execute after wake-up from any enabled interrupt:
	// sec tick (TIMER2_OVF), UART (USART0_UDRE, USART0_RX), rad event (INT1/PCINT1), keys (PCINT2/TIMER2_COMPA), USB dis/connect (INT0)
	while (true)
	{
		// handle UART only if enabled
		if (UART_GetEnabled())
		{
			// try to read string from UART & parse command
			char str[UART_RX_BUF_SIZE] = {0};
			if (UART_RxString(str))
			{
				CMD_Parse(str);
			}
			else // let random numbers be independent of UART
			{
				// seed PRNG sequencer with value of free running T1
				// T2 overflow is not synchronized to T1, detector interrupts are truly random
				// note: I/O clock to T1 is halted during sleep, this RNG might not be great..
				srand((unsigned int)TCNT1);
			}
		}

		// flag is set by T2 ISR every second
		if (RTC_CheckSecTick())
		{
			// reset watchdog (2s timeout)
			wdt_reset();

			// monitor HV & tube, process radiation data, logging
			RAD_EngineTick();
		
			// handle UI
			UI_CheckAlarm();
			UI_UpdateBattery();
			UI_RenderLcd();
		}
		
		// check if key was pressed
		byte key = KEYS_GetEvents();
		if (key)
		{
			UI_HandleKeys(key);
			UI_RenderLcd();
		}

		// check if USB was connected or disconnected
		if (PWR_CheckUsbEvent())
		{
			LCD_Clear();
			UI_RenderLcd();
		}

		// wait until UART is idle before entering sleep mode
		while (UART_TxBusy());
		
		// go to sleep to save power until interrupt wakes us up again
		PWR_SleepMode();
		
	} // end main loop

	return 0; // the cake is a lie
}

// initialize all hardware & firmware modules
static bool InitSystem(void)
{
	// reset watchdog ASAP after boot
	MCUSR = 0;				// clear reset flags (incl. WDRF)
	wdt_reset();
	wdt_enable(WDTO_8S);	// 8s timeout should be sufficient for init
	
	// init GPIO pins & start-up beep
	GPIO_Init();
	GPIO_SetPin(PIN_BEEP_EN, true);
	_delay_ms(100); // this also ensures that key caps are charged
	GPIO_SetPin(PIN_BEEP_EN, false);

	// init USB dis/connect detection
	PWR_Init();
	
	// global interrupt enable - needed for UART
	sei();
	
	// init UART soon after boot so it can be used for debugging
	bool cal_ok = UART_Init();
	UART_Printf("OSIRIS HW v%s FW v%s\n", HW_REV, FW_REV);
	UART_Printf("Init..\n");
	
	// also calibrate UART if yellow key held during boot
	cal_ok &= GPIO_GetPin(PIN_KEY_YEL); 

	// LCD init
 	LCD_Init();
 	LCD_Printf(1, "OSIRIS");
 	LCD_Printf(2, "HW v%s FW v%s", HW_REV, FW_REV);
	
	// init Timer2, needed for systick and RTC
	if (!RTC_InitRtc())
	{
		LCD_Clear();
		LCD_Printf(1, "RTC FAULT !!");
		UART_Printf("RTC FAULT !!");
		return false;
	}

	// run UART calibration if invalid or requested
	if (!cal_ok)
	{
		LCD_Clear();
		LCD_Printf(1, "UART cal..");
		
		bool ok = UART_Calibrate(true);
		
		LCD_Printf(2, ok?"OK!":"ERROR!");
		_delay_ms(1000); // make it readable
	}
	
	// initialize ADC
	if (!ADC_Init())
	{
		LCD_Clear();
		LCD_Printf(1, "ADC FAULT !!");
		UART_Printf("ADC FAULT !!");
		return false;
	}

	// enable & check radiation detection circuit
	if (!RAD_Init())
	{
		LCD_Clear();
		LCD_Printf(1, "HV FAULT !!");
		return false;
	}

	// init user interface detection
	KEYS_Init();
	UI_Init();
	
	// start T1 for RNG, prescaler 1
	SET(TCCR1B, CS10);
	
	UART_Printf("Ready!\n");
	UART_Printf("Enter '?' for help.\n");

	// enable watchdog - reset every second in main loop
	wdt_reset();
	wdt_enable(WDTO_2S);

	return true;
}

// -------------------------------------- EOF --------------------------------------
