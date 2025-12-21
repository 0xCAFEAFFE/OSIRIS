/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Application wide system functions
===============================================================================
*/

#include "sys.h"
//---------------
#include "gpio.h"
#include "pwr.h"

// handle critical fault
void SYS_Assert(bool ok)
{
#if (SYS_ASSERT_LVL==2)
	// highest escalation level -> reset
	PWR_Reset();
#elif (SYS_ASSERT_LVL==1)
	// indicate fault to user
	while (!ok)
	{
		// allow shutdown so we don't keep the device powered forever
		if (!GPIO_GetPin(PIN_KEY_RED)) { PWR_Shutdown(); }
			
		// warn user
		//GPIO_SetPin(PIN_BEEP_EN, true);
		_delay_ms(50);
		GPIO_SetPin(PIN_BEEP_EN, false);
		_delay_ms(200);
		wdt_reset();
	}
#elif (SYS_ASSERT_LVL==0)
	// asserts disabled
#else
	#error
#endif

}

// handle unhandled interrupts
ISR(BADISR_vect)
{
	SYS_EXCEPTION();
}



// -------------------------------------- EOF --------------------------------------