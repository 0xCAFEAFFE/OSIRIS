/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Interrupt & timer based implementation of key handling
===============================================================================
*/

#include "keys.h"
//---------------
#include "gpio.h"
#include "rtc.h"
#include "uart.h"

// internal variables
static volatile byte keysPressed, keyEvent;	// set by handleKeys / stopTimeout, used in TIMER2_COMPA_vect

// internal function prototypes
static void StartTimeout(void);
static void StopTimeout(void);
static byte GetKeys(void);
static void HandleKeys(void);

// externally visible variables
bool KEYS_debug;

// enable required pin change interrupts
// note: debounce caps need to be charged already, otherwise events trigger immediately
void KEYS_Init(void)
{
	KEYS_debug = false;
	
	// enable PCINT21 = PD5 = KEY_GRN, PCINT22 = PD6 = KEY_YEL, PCINT23 = PD7 = KEY_RED
	PCMSK2 |= BV(PCINT21)|BV(PCINT22)|BV(PCINT23);
	CLR_FLAG(PCIFR, PCIF2);	// clear PFIC2, don't touch other PCIF flags
	SET(PCICR, PCIE2);		// enable PCINT2
}

// disable key related interrupts
void KEYS_DeInit(void)
{
	// disable T2 match interrupt but not PCINT
	// for wake up by key if supplied by USB 
	CLR(TIMSK2, OCIE2A);
	//CLR(PCICR, PCIE2);
	
	// disable yellow and green key for wake up
	PCMSK2 &= ~(BV(PCINT21)|BV(PCINT22));
}

// return & clear all active key events
byte KEYS_GetEvents(void)
{
	byte temp;
	
	// avoid key event being overheard
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		temp = keyEvent;
		keyEvent = 0;
	}
				
	if (KEYS_debug && temp) { UART_Printf("KEY: %d\n", temp); }
	
	return temp;
}

// key press / release handler, called from PCINT2 ISR
// no multi-key handling, multiple timers match interrupts would be needed
static void HandleKeys(void)
{
	static byte keys_old;
	
	// determine which keys got pressed & which were released
	byte keys_new = GetKeys();
	byte keys_changed = keys_old ^ keys_new;
	byte keys_pressed = keys_changed & keys_new ;
	byte keys_released = (keys_changed & keys_old) & keysPressed;
	keys_old = keys_new;
	
	// ignore key press event if another key is still held (no rollover)
	if (!keysPressed)
	{
		// go through keys, prioritizes RED > YEL > GRN
		for (byte key=KEY_RED_MASK; key>0; key>>=1)
		{
			if (key & keys_pressed)
			{
				keysPressed = key;
				StartTimeout(); // if key is still pressed after timeout, it's considered a long press
				break;			// ignore other keys
			}
		}
	}
	
	// stop timeout only if valid key release event was detected
	if (!keys_released) { return; }
	else { StopTimeout(); }

	// process valid key release events
	keyEvent |= keys_released;
}

// merge individual key states to a key vector byte, invert logic polarity
static byte GetKeys(void)
{
	return 0x07 ^ (GPIO_GetPin(PIN_KEY_RED)*KEY_RED_MASK
				  |GPIO_GetPin(PIN_KEY_YEL)*KEY_YEL_MASK
				  |GPIO_GetPin(PIN_KEY_GRN)*KEY_GRN_MASK);
}

// start a 1s timeout that triggers TIMER2_COMPA_vect after expiration
// T2 wraps around exactly once every second
static void StartTimeout(void)
{
	OCR2A = TCNT2 + 0xff;	// byte value overflow is intentional
	CLR_FLAG(TIFR2, OCF2A);	// clear match interrupt flag
	SET(TIMSK2, OCIE2A);	// enable match interrupt
}

// disable T2 match interrupt
static void StopTimeout(void)
{
	CLR(TIMSK2, OCIE2A);
	keysPressed = 0;
}

// T2 counter match ISR for long key detection
ISR(TIMER2_COMPA_vect)
{
	// if the key that was pressed is still being held down -> long key press
	byte keys_still_pressed = keysPressed & GetKeys();
	keyEvent |= (keys_still_pressed << KEY_LONG_SHIFT);
	StopTimeout();
}

// pin change interrupt 2 ISR; enabled: PCINT21, PCINT22, PCINT23
ISR(PCINT2_vect)
{
	// process key related events
	HandleKeys();
}

// -------------------------------------- EOF --------------------------------------