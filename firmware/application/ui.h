/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Interface for user interface functions
===============================================================================
*/

#ifndef UI_H_
#define UI_H_

#include "sys.h"

typedef enum
{
	UI_VIEW_DOSE_RATE	= 0u,
	UI_VIEW_TOTAL_DOSE	= 1u,
	UI_VIEW_TIME		= 2u,
	UI_VIEW_ALARM		= 3u,
	UI_VIEW_VOLTS		= 4u,
	UI_NUM_VIEW_MODES	= 5u,
	UI_VIEW_FAULT		= 6u, // this one is not accessible by keys
} UI_viewMode_t;

// externally visible variables
#define UI_ALARM_LVL_NUM	5u
extern const float UI_alarmLvls[UI_ALARM_LVL_NUM];
extern bool UI_clickEnable;
extern UI_viewMode_t UI_viewMode;
extern float UI_alarmLevel;

// public function declarations
void UI_Init(void);
void UI_HandleKeys(byte key);
void UI_RenderLcd(void);
void UI_UpdateBattery(void);
void UI_CheckAlarm(void);
void UI_EmitBeep(uint16_t ms);

#endif /* UI_H_ */