/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Public interface for analog measurement functions
===============================================================================
*/

#ifndef ADC_H_
#define ADC_H_

#include "sys.h"

// public function declarations
bool ADC_Init(void);
uint16_t ADC_GetVsys(void);
uint16_t ADC_GetVbat(void);

#endif /* ADC_H_ */