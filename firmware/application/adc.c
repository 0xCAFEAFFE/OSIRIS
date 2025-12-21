/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Implementation of analog measurement functions
===============================================================================
*/

#include "adc.h"

// internal defines
#define ADC_MAX			1023u	// [LSBs]
#define ADC_V_BG		1100u	// [mV]; internal bandgap voltage
#define ADC_V_REF		5000u	// [mV]; ADC reference / system voltage
#define ADC_T_BG		80u		// [us]; bandgap settling time; typ 40µs, max 70µs
#define ADC_VREF_ERR	200u	// [mV]; maximum VREF error

// ADC input multiplexer settings
typedef enum
{
	CH_VBAT = 0x7, // PE3 = ADC7
	CH_VSYS = 0xe,	// VBG
	CH_GND	= 0xf	// GND
} ADC_Mux_t;

// ADC averaging settings
typedef enum
{
	AVG_OFF	= 0u,
	AVG_2	= 1u,
	AVG_4	= 2u,
	AVG_8	= 3u,
	AVG_16	= 4u,
	AVG_32	= 5u,
	AVG_64	= 6u,
	AVG_128 = 7u,
} ADC_Avg_t;

// internal function prototypes
static uint16_t ReadSingle(void);
static uint16_t ReadAvg(ADC_Mux_t mux, ADC_Avg_t avg);

// initialize ADC
bool ADC_Init(void)
{
	ADCSRA |= 0x03;			// select clock prescaler 64
	SET(ADMUX, REFS0);		// select AVCC as reference
	
	// check reference voltage
	uint16_t vsys = ADC_GetVsys();
	return (abs(vsys - (uint16_t)ADC_V_REF) < ADC_VREF_ERR);
}

// measure system voltage in mV
uint16_t ADC_GetVsys(void)
{
	// calculation in 32bit to avoid 16bit overflow
	uint32_t lsbs = ReadAvg(CH_VSYS, AVG_16);
	return (uint16_t)((uint32_t)ADC_MAX*ADC_V_BG/lsbs);
}

// measure battery voltage in mV
uint16_t ADC_GetVbat(void)
{
	uint32_t lsbs = ReadAvg(CH_VBAT, AVG_16);
	return (uint16_t)(lsbs*(uint32_t)ADC_V_REF/ADC_MAX);
}

// select ADC input, measure & average, returns LSBs
static uint16_t ReadAvg(ADC_Mux_t mux, ADC_Avg_t avg)
{
	ADMUX &= 0xf0;		// clear ADC input selection
	ADMUX |= mux;		// select new ADC input channel
	SET(ADCSRA, ADEN);	// enable ADC
	
	// wait for internal bandgap reference to settle
	//if (mux == CH_VSYS) { _delay_us(ADC_T_BG); }
	// not needed if dummy read is long enough
	
	// dummy read after changing channel
	static byte mux_old = CH_GND;
	if (mux != mux_old) 
	{
		mux_old = mux;
		(void)ReadSingle();
	}

	// measure & sum
	uint32_t adc_avg = 0;
	for (byte i=0; i<(1<<avg); i++) { adc_avg += ReadSingle(); }
	
	CLR(ADCSRA, ADEN);			// disable ADC to save power
	return (adc_avg >> avg);	// return average LSBs, ignore rounding error
}

// run a single ADC measurement
static uint16_t ReadSingle(void)
{
	SET(ADCSRA, ADSC);				// start conversion
	while (ADCSRA & BV(ADSC));		// busy wait for conversion to end
	return ADCW;					// return LSB value
}

// -------------------------------------- EOF --------------------------------------