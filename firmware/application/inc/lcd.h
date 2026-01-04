/*
 * Copyright (c) 2014 by ELECTRONIC ASSEMBLY <technik@lcd-module.de>
 * EA DOGM-Text (ST7036) software library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 *
 * Modified beyond recognition by Nicolai Sawilla for Project OSIRIS in 2022
 */

#ifndef LCD_H_
#define LCD_H_

#include "sys.h"

typedef enum
{
	LCD_BAT_EMPTY		= 0,	// battery empty - 0/5 bars - frame only
	// 1-4 are intermediate battery states
	LCD_BAT_FULL		= 5,	// battery full - 5/5 bars
	LCD_USB_CONNECTED	= 6,	// USB connected
	LCD_KEY_LOCK		= 7,	// keys locked
	LCD_NUM_SYMBOLS		= 8,	// number of symbols
} LCD_Symbol_t;

// public function declarations
void LCD_Init(void);
void LCD_Enable(bool on);
void LCD_Clear(void);
void LCD_Printf(uint8_t line, const char *formatstr, ...);
void LCD_PrintChar(uint8_t line, uint8_t column, char character);
void LCD_Position(uint8_t line, uint8_t column);

#endif /* LCD_H_ */