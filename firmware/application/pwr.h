/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Public interface for power related functions
===============================================================================
*/


#ifndef PWR_H_
#define PWR_H_

#include "sys.h"

void PWR_Init(void);
bool PWR_CheckUsbEvent(void);
void PWR_SleepMode(void);
void PWR_Reset(void);
void PWR_Shutdown(void);
void PWR_DeInit(void);

#endif /* PWR_H_ */