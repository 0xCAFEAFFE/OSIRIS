/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Implementation of RTC functions using Timer2 with 32kHz XTAL
===============================================================================
*/

#include "rtc.h"
//---------------
#include "rad.h"
#include "uart.h"

// internal variables
static bool secTickFlag;			// auxiliary T2 overflow flag, set by T2 OVF ISR
static volatile uint32_t rtcUptime;	// uptime in sec, incremented by T2 OVF ISR
static int32_t rtcOffset;			// offset to match set system time to uptime

// internal function prototypes
static RTC_Time_t CalcSysTime(int32_t secs);
static int32_t CalcSecTime(RTC_Time_t time);

// setup T2 for use with external 32kHz clock XTAL, triggers T2 OVF ISR every second
bool RTC_InitRtc(void)
{
	SET(ASSR, AS2);					// set T2 to asynchronous mode
	TCNT2 = 0;						// reset T2 counter
	CLR_FLAG(TIFR2, TOV2);			// clear T2 overflow flag
	TCCR2B = BV(CS20)|BV(CS22);		// enable T2, prescaler 128 -> overflow every 1s
	
	// XTAL startup should not take longer than 30*100ms=3s
	for (byte i=0; i<30; i++)
	{
		_delay_ms(100);				// check again every 100ms
		if (GET(TIFR2, TOV2))
		{
			CLR_FLAG(TIFR2, TOV2);	// clear overflow flag
			secTickFlag = false;	// clear auxiliary overflow flag
			SET(TIMSK2, TOIE2);		// enable T2 overflow interrupt
			return true;			// T2 init successful
		}
	}

	return false;	// timeout -> T2 init failed
}

// disable all interrupts and stop T2
void RTC_DeInit(void)
{
	CLR(TIMSK2, TOIE2);
	CLR(TIMSK2, OCIE2B);
	CLR(TIMSK2, OCIE2A);
	CLR(ASSR, AS2);
}

// measure actual RC oscillator frequency by comparing T1 (RC osc) and T2 (32kHz XTAL)
// T2 must already be running, function call takes ~3s, take care of watchdog outside!
// f=8MHz/256=31250Hz, lowest possible prescaler for 16bit counter for high accuracy
uint32_t RTC_GetRcOscFreq(void)
{
	uint32_t ret;
	
	CLR(TIMSK2, TOIE2);				// disable T2 overflow interrupt
	byte t1_conf = TCCR1B;			// backup T1 settings
	TCCR1B = 0;						// stop T1
	TCNT1 = 0;						// reset T1
	
	while (!(GET(TIFR2, TOV2)));	// busy wait for first T2 overflow
	CLR_FLAG(TIFR2, TOV2);			// clear overflow flag
	SET(TCCR1B, CS12);				// enable T1, prescaler 256
	
	while (!(GET(TIFR2, TOV2)));	// busy wait for second T2 overflow
	CLR_FLAG(TIFR2, TOV2);			// clear overflow flag
	CLR(TCCR1B, CS12);				// stop T1
	
	SET(TIMSK2, TOIE2);				// re-enable T2 overflow interrupt
	ret = 256 * (uint32_t)TCNT1;	// calculate RC oscillator frequency
	TCCR1B = t1_conf;				// restore T1 settings
	rtcUptime += 2;					// preserve uptime integrity
	secTickFlag = true;				// set auxiliary flag

	return ret;
}

// get raw uptime in seconds, this stays the same even if systime is changed
uint32_t RTC_GetUpTime(void)
{
	return rtcUptime;
}

// set h:m:s systime
void RTC_SetSysTime(RTC_Time_t time)
{	
	// changing rtcUptime directly would require blocking the IRQ, use an offset instead
	rtcOffset = CalcSecTime(time) - rtcUptime ;
}

// get systime in seconds
// if used for timestamps, changing systime will mess things up
uint32_t RTC_GetSecTime(void)
{
	return rtcOffset + rtcUptime;
}

// get systime in h:m:s format
RTC_Time_t RTC_GetSysTime(void)
{
	return CalcSysTime(RTC_GetSecTime());
}

// returns true if one second mark expired
bool RTC_CheckSecTick(void)
{
	if (secTickFlag)
	{
		secTickFlag = false;
		return true;
	}
	
	return false;
}

// calculate systime in h:m:s format from raw seconds
static RTC_Time_t CalcSysTime(int32_t secs)
{
	RTC_Time_t time;
	time.hours = secs / 3600;
	secs %= 3600;
	time.mins = secs / 60;
	time.secs = secs % 60;
	return time;
}

// calculate systime in seconds from h:m:s format
static int32_t CalcSecTime(RTC_Time_t time)
{
	return (int32_t)time.hours*3600 + (int32_t)time.mins*60 + (int32_t)time.secs;
}

// T2 overflow ISR, triggered every second
ISR(TIMER2_OVF_vect)
{
	// increment raw second counter
	rtcUptime++;
	secTickFlag = true;
	
	// save copy of raw counter variable
	RAD_UpdateBuffer();
}

// -------------------------------------- EOF --------------------------------------
