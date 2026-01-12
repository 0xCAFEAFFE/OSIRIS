/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Implementation of user interface functions
===============================================================================
*/

#include "ui.h"
//---------------
#include "adc.h"
#include "gpio.h"
#include "keys.h"
#include "lcd.h"
#include "pwr.h"
#include "rad.h"
#include "rtc.h"
#include "uart.h"

// internal defines
#define UI_SOUND_DISABLE		0	// 0=default, 1=disable beeper
#define UI_BAT_WARN_INTERVAL	5u	// [s]; time between beeps for low battery warning
#define UI_VBAT_UNDEFINED		-1	// valid battery voltage values are positive

// externally visible variables
const float UI_alarmLvls[UI_ALARM_LVL_NUM] = {0.5f, 1.0f, 2.0f, 5.0f, 0.0f}; // alarm levels in µSv/h
bool UI_clickEnable;
UI_viewMode_t UI_viewMode;
float UI_alarmLevel;

// internal variables
static byte batSymbol, filterLevel;
static bool alarmAck, keyLock, alarmEn, batLow;

// initialize user interface
void UI_Init(void)
{
	// reset public variables
	UI_alarmLevel = UI_alarmLvls[0]; // 0.5µSv/h with fast filter should not trigger alarm at normal background levels
	UI_clickEnable = false;
	UI_viewMode = UI_VIEW_DOSE_RATE;
}

// handle keys, USB dis/connect and charging
void UI_HandleKeys(byte key)
{
	// key lock active - ignore all but yellow long
	if (keyLock && key&(~KEY_YEL_LONG)) { return; }
	
	// alarm active - ignore all but red short
	if (alarmEn && (!alarmAck) && (key&(~KEY_RED_SHORT))) { return; }

	// handle keys
	switch (key)
	{
		case KEY_GRN_SHORT:
		{
			// cycle through view modes
			if (++UI_viewMode >= UI_NUM_VIEW_MODES) { UI_viewMode = 0; }
			break;
		}
		case KEY_GRN_LONG:
		{
			// function of long green key press is dependend on current mode
			switch (UI_viewMode)
			{
				case UI_VIEW_DOSE_RATE:
				{
					// cycle through dose rate filter setting
					if (++filterLevel >= RAD_FILTER_LVL_NUM) { filterLevel = 0; }
					RAD_filterFactor = RAD_filterLvls[filterLevel];
					break;
				}
				case UI_VIEW_ALARM:
				{
					// cycle through alarm levels
					static byte a;
					if (++a >= UI_ALARM_LVL_NUM) { a = 0; }
					UI_alarmLevel = UI_alarmLvls[a];
					break;
				}
				case UI_VIEW_TOTAL_DOSE:
				{
					// reset total accumulated dose
					RAD_SetTotalDose(0.0f);
					break;
				}
				default: { break; }
			}
			
			LCD_Clear();
			break;
		}
		case KEY_YEL_SHORT:
		{
			// toggle clicker
			UI_clickEnable ^= true;
			GPIO_SetPin(PIN_CLICK_EN, UI_clickEnable);
			LCD_Clear();
			break;
		}
		case KEY_YEL_LONG:
		{
			// toggle key lock
			keyLock ^= true;
			LCD_Clear();
			break;
		}
		case KEY_RED_SHORT:
		{
			// acknowledge alarm
			alarmAck = true;
			break;
		}
		case KEY_RED_LONG:
		{
			// shutdown
			PWR_Shutdown();
			break;
		}
	}
}

// update battery symbol, charging animation, connection symbol
void UI_UpdateBattery(void)
{
	static byte b; // charging animation state
	static int16_t vBat = UI_VBAT_UNDEFINED;
	
	// check if connected to USB
	if (GPIO_GetPin(PIN_VUSB))
	{
		// battery charging?
		if (GPIO_GetPin(PIN_BAT_STAT))
		{
			// battery full
			batSymbol = LCD_BAT_FULL;
		}
		else
		{
			// display charging animation
			batSymbol = b;
			b = (b == LCD_BAT_FULL) ? LCD_BAT_EMPTY : (b+1);
		}
		// no use in measuring battery voltage while charging
		vBat = UI_VBAT_UNDEFINED;
		batLow = false;
	}
	else // running on battery
	{
		// reset charging animation
		b = 0;
		
		// measure & update battery state every minute
		if (!RTC_GetSysTime().secs || (vBat==UI_VBAT_UNDEFINED))
		{
			vBat = ADC_GetVbat();

			if (vBat>4050)		 { batSymbol = LCD_BAT_FULL;	}
			else if (vBat>3900)	 { batSymbol = 4; }
			else if (vBat>3750) { batSymbol = 3; }
			else if (vBat>3600)	 { batSymbol = 2; }
			else if (vBat>3450) { batSymbol = 1; }
			else { batSymbol = LCD_BAT_EMPTY; }
			
			// indicate low battery to user
			batLow = (batSymbol == LCD_BAT_EMPTY);

			// shutdown if battery critical
			if (vBat < 3.3f)
			{
				LCD_Clear();
				LCD_Printf(1, "BATTERY EMPTY!");
				LCD_Printf(2, "Vbat=%u", vBat);
				_delay_ms(1000);
				PWR_Shutdown();
			}
		}
	}
}

// call this every second
void UI_CheckAlarm(void)
{
	float rate = RAD_GetDoseRate();
	RTC_Time_t time = RTC_GetSysTime();

	// check for alarm conditions
	if (RAD_GetFault())
	{
		// detector fault
		alarmEn = true;
			
		// indicate alarm if not already acknowledged
		if (alarmAck) { GPIO_SetPin(PIN_BEEP_EN, false); }
		else
		{
			UI_viewMode = UI_VIEW_FAULT;
			GPIO_TogglePin(PIN_BEEP_EN);
		}
	}
	else
	{
		alarmEn = (UI_alarmLevel != 0.0f) && (rate > UI_alarmLevel);
		if (alarmEn)
		{
			// dose rate alarm
			UART_Printf("%02u:%02u:%02u ", time.hours, time.mins, time.secs);
			UART_Printf("Dose Rate Alert! %.3fuSv/h\n", (double)rate);

			if (alarmAck) { GPIO_SetPin(PIN_BEEP_EN, false); }
			else
			{
				UI_viewMode = UI_VIEW_DOSE_RATE;
				GPIO_TogglePin(PIN_BEEP_EN);
			}
		}
		else
		{
			// reset alarm
			LCD_Clear();
			GPIO_SetPin(PIN_BEEP_EN, false);
			alarmAck = false;
			alarmEn = false;
		}
	}
	
	// emit periodic low battery warning
	if (batLow && !(RTC_GetUpTime()%UI_BAT_WARN_INTERVAL)) { UI_EmitBeep(10); }
}

// call this if display needs to be updated
void UI_RenderLcd(void)
{
	float rate = RAD_GetDoseRate();
	RTC_Time_t time = RTC_GetSysTime();
	
	// clearing screen produces visible delay -> only if screen changes
	static int8_t old_view = UI_NUM_VIEW_MODES; // force update on first run
	if (UI_viewMode != old_view)
	{
		LCD_Clear();
		old_view = UI_viewMode;
	}
	
	// handle view modes
	switch (UI_viewMode)
	{
		case UI_VIEW_DOSE_RATE: // current dose rate
		{
			LCD_Printf(1, "Dose Rate:");
			
			if (rate < 10.0f)		 { LCD_Printf(2, "%.3fuSv/h",(double)rate); }
			else if (rate < 100.0f)	 { LCD_Printf(2, "%.2fuSv/h",(double)rate); }
			else if (rate < 1000.0f) { LCD_Printf(2, "%.1fuSv/h",(double)rate); }
			else /* >1000 */		 { LCD_Printf(2, "%.2fmSv/h",(double)rate/1000); }
#warning "maybe we can just overwrite the latest digits with blank spaces?"
			// display alarm status
			LCD_Position(2, 11);
			if (alarmEn) { LCD_Printf(0, " !!!"); }
			
			break;
		}
		case UI_VIEW_TOTAL_DOSE: // total dose
		{
			LCD_Printf(1, "Total Dose:");
			float dose = RAD_GetTotalDose();
			
			if (dose < 10.0f)		 { LCD_Printf(2, "%.3fuSv",(double)dose); }
			else if (dose < 100.0f)	 { LCD_Printf(2, "%.2fuSv",(double)dose); }
			else if (dose < 1000.0f) { LCD_Printf(2, "%.1fuSv",(double)dose); }
			else /* >1000 */		 { LCD_Printf(2, "%.2fmSv",(double)dose/1000); }
			
			break;
		}
		case UI_VIEW_TIME: // uptime
		{
			LCD_Printf(1, "Time:");
			LCD_Printf(2, "%02u:%02u:%02u",time.hours,time.mins,time.secs);
			
			break;
		}
		case UI_VIEW_VOLTS: // voltages
		{
			double vs = ADC_GetVsys()/1000.0f;
			double vb = ADC_GetVbat()/1000.0f;
			
			LCD_Printf(1, "Voltages:");
			LCD_Printf(2, "Vs=%.2f Vb=%.2f", vs, vb);
			
			break;
		}
		case UI_VIEW_ALARM: // alarm level
		{
			LCD_Printf(1, "Alarm Lvl:");

			if (UI_alarmLevel) { LCD_Printf(2, "%.1fuSv/h", (double)UI_alarmLevel); }
			else { LCD_Printf(2, "Off"); }
			
			break;
		}
		case UI_VIEW_FAULT:
		{
			LCD_Printf(1, "RAD FAULT !!!");

			break;
		}
		default: { SYS_EXCEPTION(); }
	}
	
	// display status icons
	static bool batSymShow = true;
	if (batLow) { batSymShow ^= true; }
	else { batSymShow = true; }
	LCD_PrintChar(1, 16, batSymShow ? batSymbol : ' ');	
	
	if (GPIO_GetPin(PIN_VUSB)) { LCD_PrintChar(1, 15, LCD_USB_CONNECTED); }
	if (UI_clickEnable) { LCD_PrintChar(1, 14, 'c'); }
	if (keyLock) { LCD_PrintChar(1, 13, LCD_KEY_LOCK); }
	
	// only display filter setting in dose rate mode
	if (UI_viewMode == UI_VIEW_DOSE_RATE)
	{
		if (filterLevel == 0) { LCD_PrintChar(2, 16, 'F'); }
		else if (filterLevel == 1) { LCD_PrintChar(2, 16, 'M'); }
		else if (filterLevel == 2) { LCD_PrintChar(2, 16, 'S'); }
		else { SYS_EXCEPTION(); }
	}
	else { LCD_PrintChar(2, 16, ' '); }
}

// interrupt controlled beep emit
void UI_EmitBeep(uint16_t ms)
{
#if (UI_SOUND_DISABLE)
	(void)ms;
	return;
#else

	// enable beeper, disabled in ISR
	GPIO_SetPin(PIN_BEEP_EN, true);
	
	// calculate T2 ticks
	byte ticks = (byte)roundf((float)ms/3.90625f);
	
	OCR2B = TCNT2 + ticks;	// set timeout
	CLR_FLAG(TIFR2, OCF2B);	// clear match interrupt flag
	SET(TIMSK2, OCIE2B);	// enable match interrupt

#endif
}

// T2 counter match ISR for sound off
ISR(TIMER2_COMPB_vect)
{
	CLR(TIMSK2, OCIE2B);	// disable interrupt
	GPIO_SetPin(PIN_BEEP_EN, false);
}


// -------------------------------------- EOF --------------------------------------
