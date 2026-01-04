/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Public interface for UART functions
===============================================================================
*/

#ifndef UART_H_
#define UART_H_

#include "sys.h"

// 28.8kbd is the max rate, the RC oscillator runs at 7.6-8.4MHz and the CH340 allows <2% error
// Note: 28.8kbd is not a standard baud rate and no longer mentioned in the latest CH340 datasheet but seems to be working fine anyways
#define UART_BAUDRATE			28800u

#define UART_TX_BUF_SIZE		256u
#define UART_PRINT_BUF_SIZE		64u
#define UART_RX_BUF_SIZE		32u

// public function declarations
bool UART_Init(void);
void UART_Printf(const char *formatstr, ...);
bool UART_RxString(char* buffer);
bool UART_TxBusy(void);
byte UART_CalcUbrr(uint32_t f_real);
bool UART_SetUbrr(byte ubrr, bool write_to_eep);
void UART_Enable(bool en);
bool UART_VerifyUbrr(byte ubrr);
void UART_PrintTimestamp(void);
bool UART_GetEnabled(void);
bool UART_Calibrate(bool write_to_eeprom);

#endif /* UART_H_ */