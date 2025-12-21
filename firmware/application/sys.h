/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Application wide defines, macros and settings
===============================================================================
*/

#ifndef SYS_H_
#define SYS_H_

#define F_CPU 8000000UL		// using internal 8Mhz RC oscillator, needed for delay.h

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

// public defines
#define SYS_ASSERT_LVL	1u	// assert handling strategy: 0=off, 1=warn, 2=reset
#define SYS_EXCEPTION() SYS_Assert(false)

// revision defines
#define HW_REV	"2.0"
#define FW_REV	"2.1"

// logic defines
#define IN	false
#define OUT true
#define LO	false
#define HI	true

// bit macros
#define BV(bit)				_BV(bit) // from sfr_defs.h
#define SET(reg, bit)		(reg |= BV(bit))
#define GET(reg, bit)		((bool)(reg & BV(bit)))
#define CLR(reg, bit)		(reg &= ~BV(bit))
#define INV(reg, bit)		(reg ^= BV(bit))

// don't OR interrupt flags to avoid clearing other flags unintentionally!
// Warning: don't use this macro for UCSR0A though, since it contains not only flags but settings as well..
#define CLR_FLAG(reg, bit)	(reg = BV(bit))

// type definitions
typedef uint8_t byte;
typedef volatile uint8_t sfr;

// public function declarations
void SYS_Assert(bool ok);

#endif /* SYS_H_ */