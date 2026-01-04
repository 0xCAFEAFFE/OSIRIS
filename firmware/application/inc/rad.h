/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Public interface for radiation detection functions
===============================================================================
*/

#ifndef RAD_H_
#define RAD_H_

#include "sys.h"

// externally visible variables
#define RAD_FILTER_LVL_NUM	3u
extern const float RAD_filterLvls[RAD_FILTER_LVL_NUM];
extern float RAD_filterFactor;
extern uint16_t RAD_uartLogInterval;
extern float RAD_totalDose;

// public function declarations
bool RAD_Init(void);
bool RAD_CheckHv(uint16_t *counts);
bool RAD_DetectorCheck(void);
bool RAD_GetFault(void);
void RAD_EngineTick(void);
void RAD_UpdateBuffer(void);
float RAD_GetDoseRate(void);
void RAD_DeInit(void);

#endif /* RAD_H_ */