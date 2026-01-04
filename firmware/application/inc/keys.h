/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Public interface for key handling functions
===============================================================================
*/

#ifndef KEYS_H_
#define KEYS_H_

#include "sys.h"

// define masks for key event handling
#define	KEY_GRN_MASK	1u
#define	KEY_YEL_MASK	2u
#define	KEY_RED_MASK	4u

#define KEY_LONG_SHIFT	3u

#define KEY_GRN_SHORT	(KEY_GRN_MASK)
#define KEY_YEL_SHORT	(KEY_YEL_MASK)
#define KEY_RED_SHORT	(KEY_RED_MASK)
#define KEY_ALL_SHORT	(KEY_GRN_SHORT|KEY_YEL_SHORT|KEY_RED_SHORT)

#define KEY_GRN_LONG	(KEY_GRN_MASK<<KEY_LONG_SHIFT)
#define KEY_YEL_LONG	(KEY_YEL_MASK<<KEY_LONG_SHIFT)
#define KEY_RED_LONG	(KEY_RED_MASK<<KEY_LONG_SHIFT)
#define KEY_ALL_LONG	(KEY_GRN_LONG|KEY_YEL_LONG|KEY_RED_LONG)

#define KEY_ALL			(KEY_ALL_SHORT|KEY_ALL_LONG)

// externally visible variables
extern bool KEYS_debug;

// public function declarations
void KEYS_Init(void);
byte KEYS_GetEvents(void);
void KEYS_DeInit(void);

#endif /* KEYS_H_ */