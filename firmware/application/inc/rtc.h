/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Public interface for RTC functions
===============================================================================
*/

#ifndef TIMER_H_
#define TIMER_H_

#include "sys.h"

typedef struct
{
	uint8_t secs;
	uint8_t mins;
	uint16_t hours;	// overflow after ~7.5y
} RTC_Time_t;

// public function declarations
bool RTC_InitRtc(void);
uint32_t RTC_GetRcOscFreq(void);
uint32_t RTC_GetUpTime(void);
void RTC_SetSysTime(RTC_Time_t time);
RTC_Time_t RTC_GetSysTime(void);
uint32_t RTC_GetSecTime(void);
bool RTC_CheckSecTick(void);
void RTC_DeInit(void);

#endif /* TIMER_H_ */