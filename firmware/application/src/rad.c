/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Implementation of radiation detection functions
===============================================================================
*/

#include "rad.h"
//---------------
#include "gpio.h"
#include "rtc.h"
#include "uart.h"

// internal defines
#define RAD_DEBUG				0		// 0=off, 1=print longest interval between pulses
#define RAD_MAX_PULSE_INTERVAL	60u		// [s]; no time >30s was observed between pulses in ~12h, double it just in case
#define RAD_HV_MIN_PULSES		10u		// typically 25 edges in 100ms at background levels, leave some margin
#define RAD_HV_MAX_PULSES		500u	// theoretical maximum at full load

// SBM20: 190us dead time, incl. amp: 210uS -> 60s/200us=300kHz, avoid div/0 by choosing lower value
#define RAD_DEAD_TIME		190e-6f

// conversion factor CPM -> uSv/h, for SBM-20 GM tube
// 175 is widely used in amateur projects, 220 matches official background radiation data, 210 seems to be valid for Cs-137
#define RAD_CONV_FACTOR		215.0f

// externally visible variables
const float RAD_filterLvls[RAD_FILTER_LVL_NUM] = {0.2f, 0.05f, 0.02f}; // exponential smoothing coefficients
float RAD_filterFactor;
uint16_t RAD_uartLogInterval;

// internal variables
static volatile uint16_t hvCounts;	// incremented in PCINT1 ISR
static volatile uint16_t rawCounts;	// incremented INT1 ISR
static uint64_t totalCounts;		// max: 1.43GSv - should be sufficient for a while
static uint16_t countBuffer;
static float doseRate;
static bool radFault;

// internal function prototypes
static void ProcessData(void);

// start & monitor high voltage supply & detector tube
bool RAD_Init(void)
{
	// reset public variables
	RAD_filterFactor = RAD_filterLvls[0]; // start with fast filter
	RAD_uartLogInterval = 0;
	
	// read total counts from EEPROM
	while (!eeprom_is_ready());
	float dose = eeprom_read_float((const float*)RAD_DOSE_EEP_ADDR);
	RAD_SetTotalDose(dose);
	
	// enable & check high voltage power supply
	GPIO_SetPin(PIN_HV_EN, true);
	if (!RAD_CheckHv(NULL)) { return false; }
	
	// clear & enable INT1 (falling edge on PIN_PULSE_INT)
	// according to the datasheet, external edge interrupts can't be used to wake up from power save mode
	// this is a mistake in the documentation, see here: http://gammon.com.au/interrupts
	SET(EICRA, ISC11);
	CLR_FLAG(EIFR, INTF1);
	SET(EIMSK, INT1);

	return true;
}

// disable interrupts and HV supply
void RAD_DeInit(void)
{
	GPIO_SetPin(PIN_HV_EN, false);
	CLR(EIMSK, INT1);
	CLR(PCICR, PCIE1);
}

// monitor high voltage boost converter for malfunction
// returns false if too many or too little pulses are detected in given time
bool RAD_CheckHv(uint16_t *counts)
{
	// reset counter variable
	hvCounts = 0;
	
	// clear & enable PCINT12 = PC4 = PIN_HV_GATE on PCINT1
	SET(PCMSK1, PCINT12);
	CLR_FLAG(PCIFR, PCIF1);
	SET(PCICR, PCIE1);
	
	// count number of edges in 100ms
	_delay_ms(100);
	CLR(PCICR, PCIE1); // disable HV monitor interrupt
	if (counts != NULL) { *counts = hvCounts; }
	
	// HV gate pin should see a reasonable number of pulses
	return (hvCounts>RAD_HV_MIN_PULSES) && (hvCounts<RAD_HV_MAX_PULSES);
}

// return false if counter is suspiciously silent
bool RAD_DetectorCheck(void)
{
	static uint16_t counts_old;
	static uint32_t last_pulse_timestamp;
	
	if (countBuffer != counts_old)
	{
		uint32_t uptime = RTC_GetUpTime();

#if (RAD_DEBUG)
		static uint32_t longest_pause;
		uint32_t pause = uptime - last_pulse_timestamp;
		if (pause > longest_pause)
		{
			longest_pause = pause;
			UART_Printf("t_up: %u, p_max: %u\n", (uint16_t)uptime, (uint16_t)longest_pause);
		}
#endif
		// new radiation event detected
		counts_old = countBuffer;
		last_pulse_timestamp = uptime;
	}
	else if (RTC_GetUpTime() > (last_pulse_timestamp + RAD_MAX_PULSE_INTERVAL))
	{
		// timeout, no pulses detected at all -> HV supply, tube or pulse amp might be defective
		return false;
	}
	
	return true;
}

// monitor HV & tube, process radiation data, handle logging
void RAD_EngineTick(void)
{
	RTC_Time_t time = RTC_GetSysTime();

	// no pulses detected in a long time
	if (!RAD_DetectorCheck())
	{
		// print error only once
		if (radFault) { return; }
		radFault = true;
		
		UART_Printf("%02u:%02u:%02u ", time.hours, time.mins, time.secs);
		
		// check HV driver signal
		uint16_t counts;
		if (!RAD_CheckHv(&counts))
		{
			GPIO_SetPin(PIN_HV_EN, false);
			UART_Printf("HV FAULT !!! %u\n", counts);
		}
		else // if HV supply is OK -> detector must be defective
		{
			UART_Printf("DETECTOR FAULT !!!\n");
		}
		
		return; // nothing else to do
	}
	else if (radFault)
	{
		// detector fault recovered
		radFault = false;	
		UART_Printf("%02u:%02u:%02u ", time.hours, time.mins, time.secs);
		UART_Printf("Detector recovered..\n");
	}
		
	// crunch some numbers
	ProcessData();
	
	// log radiation data to UART
	static bool log_head = false;
	if (RAD_uartLogInterval)
	{
		if (!log_head)
		{
			UART_Printf("Time     Rate       Total\n");
			log_head = true;
		}
		
		if (!(RTC_GetSecTime() % RAD_uartLogInterval))
		{
			UART_Printf("%02u:%02u:%02u ", time.hours, time.mins, time.secs);
			UART_Printf("%.3fuSv/h %.4fuSv\n", (double)doseRate, (double)RAD_GetTotalDose());
		}
	}
	else
	{
		log_head = false;
	}
}

// this hook is called from within T2 sectick ISR
// needed so we can work with the copy while the original is updated by the pulse ISR
void RAD_UpdateBuffer(void)
{
	// atomic block not actually needed if only called from ISR, just in case
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		countBuffer = rawCounts;
	}
}

// return current dose rate
float RAD_GetDoseRate(void)
{
	return doseRate;
}

// returns true if fault occurred
bool RAD_GetFault(void)
{
	return radFault;
}

// call this every second to update radiation data
// as long as this is called soon after sectick ISR, there is no risk of countBuffer being changed while this runs
static void ProcessData(void)
{
	// static local vars
	static uint16_t buffer_old;
	static float cpm_smooth;
	
	// dose rate calculation - handle intermediate buffer
	uint16_t cps = countBuffer - buffer_old;
	buffer_old = countBuffer;
	
	// dead-time correction
	float cps_corr = cps/(1.0f - cps*RAD_DEAD_TIME);
	
	// CPM estimation & exponential smoothing of CPM value
	cpm_smooth = RAD_filterFactor*(60.0f*cps_corr) + (1-RAD_filterFactor)*cpm_smooth;
	
	// CPM to dose rate conversion
	doseRate = cpm_smooth / RAD_CONV_FACTOR;
	
	// total dose calculation - use int to avoid float rounding errors
	totalCounts += (uint64_t)roundf(cps_corr);
}

// set total accumulated dose value
void RAD_SetTotalDose(float dose)
{
	totalCounts = (dose * 60.0f * RAD_CONV_FACTOR);
}

// get total accumulated dose rate
float RAD_GetTotalDose(void)
{
	return totalCounts / (60.0f * RAD_CONV_FACTOR);
}

// save total accumulated dose to EEPROM
void RAD_SaveTotalDose(void)
{
	float dose = RAD_GetTotalDose();
	while (!eeprom_is_ready());
	eeprom_update_float((float*)RAD_DOSE_EEP_ADDR, dose);
}

// INT1 external interrupt ISR (GM tube pulse event)
ISR(INT1_vect)
{
	// increment raw pulse counter, max rate is ~5kcps / ~1.5mSv/h
	// overflow is acceptable as long as counter never overflows twice before being handled by ProcessData()
	rawCounts++;
}

// HV supply monitor pin change ISR; enabled: PCINT12
ISR(PCINT1_vect)
{
	// increment HV edge counter
	hvCounts++;
}

// -------------------------------------- EOF --------------------------------------