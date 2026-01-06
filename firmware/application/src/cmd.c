/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Implementation of command parser
===============================================================================
*/

#include "cmd.h"
//---------------
#include "adc.h"
#include "gpio.h"
#include "keys.h"
#include "pwr.h"
#include "rad.h"
#include "rtc.h"
#include "sys.h"
#include "uart.h"
#include "ui.h"

// internal defines
#define CMD_ECHO	1	// received strings are sent back on TX

// command parser replies
typedef enum
{
	REPLY_OK		= 0u,	// command executed ok
	REPLY_UNKNOWN	= 1u,	// unknown command
	REPLY_DENIED	= 2u,	// value or operation not allowed for this command
	REPLY_ERROR		= 3u,	// supplied parameter is invalid
} CMD_Reply_t;

// define help text
// unused letters: gijopqwy
#define NUM_HELP_STRS	20u
#define HELP_STR_LEN	24u
// note: format specifier %S (uppercase!) must be used to printf strings from flash
static const __flash char helpStr[NUM_HELP_STRS][HELP_STR_LEN] =
{
//  "12345678901234567890123" - longest possible string: 23 chars + '\0' = 24 
	"cmd format: x -> get x",
	"x1 -> set x=1. cmds:",
	"a - alarm level",
	"b - beep emit",
	"c - clicker setting",
	"d - dose total",
	"e - EEPROM r/w @ X",
	"f - filter factor",
	"h - high voltage",
	"k - key debugging",
	"l - logging interval",
	"m - mode view",
	"n - number random",
	"r - rate dose",
	"s - shutdown",
	"t - time",
	"u - UART calibration",
	"v - voltage measure",
	"x - EEPROM address",
	"z - reset system",
};

// check if supplied string contains a valid command
// must be terminated with a newline character (LF = '\n' = 0xA)
// currently supported commands are visible in helpStr above
// command always consists of a single letter and optional argument(s)
void CMD_Parse(char* str)
{
	// local variables
	char cmd = str[0];
	byte cmd_len = strlen(str);
	CMD_Reply_t reply = REPLY_OK;
	int16_t arg_int;
	float arg_float;
	bool set = false;
	static byte x;

#if CMD_ECHO
	// UART echo - send back received string
	UART_Printf("%s\n", str);
#endif

	// data received -> set parameter
	if (cmd_len > 1)
	{
		str++;					// ignore cmd char
		arg_int = atoi(str);	// interpret as integer
		arg_float = atof(str);	// interpret as float - this needs A LOT of flash
		set = true;
	}

	// decode command char
	switch (cmd)
	{
		// ---------- alarm level  ----------
		case 'a':
		{
			if (set) { UI_alarmLevel = arg_float; }
			else { UART_Printf("%.3f\n", (double)UI_alarmLevel); }
			
			break;
		}
		
		// ---------- beeper ----------
		case 'b':
		{
			if (set) { UI_EmitBeep(arg_int); }
			else { reply = REPLY_DENIED; }
			
			break;
		}
		
		// ---------- clicker ----------
		case 'c':
		{
			if (set)
			{
				UI_clickEnable = (bool)arg_int;
				GPIO_SetPin(PIN_CLICK_EN, UI_clickEnable);
			}
			else
			{
				UART_Printf("%u\n", UI_clickEnable);
			}
			
			break;
		}

		// ---------- total dose ----------
		case 'd':
		{
			if (set) { RAD_totalDose = arg_float; }
			else { UART_Printf("%.4fuSv\n", (double)RAD_totalDose); }
			
			break;
		}

		// ---------- EEPROM test ----------
		case 'e':
		{
			// use test variable x as address
			unsigned addr = x; // dirty hack to silence compiler warning
			
			while (!eeprom_is_ready());
			if (set)
			{
				if ((arg_int>0xff) || (arg_int<0)) { reply = REPLY_ERROR; }
				else { eeprom_update_byte((uint8_t*)addr, (uint8_t)arg_int); }
			}
			else
			{
				UART_Printf("EEP[0x%02X]=0x%02X\n", x, eeprom_read_byte((uint8_t*)addr));
			}
			
			break;
		}
		
		// ---------- filter factor ----------
		case 'f':
		{
			if (set) { RAD_filterFactor = arg_float; }
			else { UART_Printf("%.3f\n", (double)RAD_filterFactor); }
			
			break;
		}
		
		// ---------- high voltage supply ----------
		case 'h':
		{
			if (set)
			{
				GPIO_SetPin(PIN_HV_EN, arg_int);
			}
			else
			{
				uint16_t count;
				RAD_CheckHv(&count);
				UART_Printf("%u\n", count);
			}
			
			break;
		}
		
		// ---------- key debug ----------
		case 'k':
		{
			if (set) { KEYS_debug = (bool)arg_int; }
			else { UART_Printf("%u\n", KEYS_debug); }
			
			break;
		}
		
		// ---------- log interval ----------
		case 'l':
		{
			if (set) { RAD_uartLogInterval = arg_int; }
			else { UART_Printf("%u\n", RAD_uartLogInterval); }
			
			break;
		}
		
		// ---------- view modes ----------
		case 'm':
		{
			if (set) { UI_viewMode = arg_int; }
			else { UART_Printf("%u\n", UI_viewMode); }
			
			break;
		}
		
		// ---------- random number mode ----------
		case 'n':
		{
			if (set) { reply = REPLY_DENIED; }
			else { UART_Printf("%u\n", rand()); } // srand is called at random intervals in main
			
			break;
		}
			
		// ---------- dose rate ----------
		case 'r':
		{
			if (set) { reply = REPLY_DENIED; }
			else { UART_Printf("%.3fuSv/h\n", (double)RAD_GetDoseRate()); }
			
			break;
		}
			
		// ---------- shutdown ----------
		case 's':
		{
			PWR_Shutdown();
			break;
		}

		// ---------- system time ----------
		case 't':
		{
			RTC_Time_t time;
			
			if (set)
			{
				reply = REPLY_ERROR;
				
				char* hours = strtok(str, ":");
				if (!hours) { break; }
				char* mins = strtok(NULL, ":");
				if (!mins) { break; }
				char* secs = strtok(NULL, ":");
				if (!secs) { break; }
				
				time.hours = atoi(hours);
				time.mins = atoi(mins);
				time.secs = atoi(secs);
				
				RTC_SetSysTime(time);
				
				reply = REPLY_OK;
			}
			else
			{
				time = RTC_GetSysTime();
				UART_Printf("%02u:%02u:%02u\n", time.hours, time.mins, time.secs);
			}
			
			break;
		}
		
		// ---------- UART calibration ----------
		case 'u':
		{
			// warning: this will create glitch in calculated dose rate
			if (set)
			{
				// parameter 0 runs calibration
				if (!arg_int)
				{	
					wdt_reset();
					wdt_enable(WDTO_4S);
					
					// apply but don't write to EEPROM
					UART_Calibrate(false);
					
					wdt_reset();
					wdt_enable(WDTO_2S);
				}
				else
				{
					if (!UART_SetUbrr(arg_int, false)) { reply = REPLY_ERROR; }
				}
			}
			else
			{
				// read current value
				UART_Printf("%u\n", UBRR0);
			}
			
			break;
		}
		
		// ---------- voltage measurements ----------
		case 'v':
		{
			if (set)
			{
				reply = REPLY_DENIED;
			}
			else
			{
				UART_Printf("Vsys=%u, Vbat=%u, %s\n", ADC_GetVsys(), ADC_GetVbat(), GPIO_GetPin(PIN_BAT_STAT) ? "full" : "charging" );
			}
			
			break;
		}
		
		// ---------- EEPROM address / test variable ----------
		case 'x':
		{
			if (set)
			{
				if ((arg_int>0xff) || (arg_int<0)) { reply = REPLY_ERROR; }
				else { x = arg_int; }
			}
			else
			{
				UART_Printf("%u\n", x);
			}
			
			break;
		}

		// ---------- reset ----------
		case 'z':
		{
			PWR_Reset();
			break;
		}

		// ---------- help ----------
		case '?':
		{
			if (set)
			{
				reply = REPLY_ERROR;
			}
			else
			{
				// print help string for each available command
				for (byte i=0; i<NUM_HELP_STRS; i++)
				{
					// format specifier %S (uppercase!) must be used to printf strings from flash
					UART_Printf("%S\n", helpStr[i]);
					while (UART_TxBusy()); // avoid TX buffer overflow
				}
			}
			
			break;
		}

		// -------------------------------------
		default: { reply = REPLY_UNKNOWN; }
	}

	// response
	switch (reply)
	{
		case REPLY_OK:
			UART_Printf("OK\n");
			break;
		case REPLY_UNKNOWN:
			UART_Printf("UNKNOWN - '?' -> help\n");
			break;
		case REPLY_ERROR:
			UART_Printf("ERROR\n");
			break;	
		case REPLY_DENIED:
			UART_Printf("DENIED\n");
			break;
		default:
			SYS_EXCEPTION();
	}
}

// -------------------------------------- EOF --------------------------------------