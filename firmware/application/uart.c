/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Implementation of interrupt based UART functions
===============================================================================
*/

#include "uart.h"
//---------------
#include "..\shared\boot.h"
#include "gpio.h"
#include "rtc.h"

// internal variables
static bool uartBusy, uartEnable, rxFrameError;
static volatile byte txBuffer[UART_TX_BUF_SIZE], rxBuffer[UART_RX_BUF_SIZE]; // ring buffers
static volatile byte rxBufIn, rxBufOut, txBufIn, txBufOut;

// internal function prototypes
static void TxChar(byte in);

// UART0 initialization - use UART_Enable() to enable or disable RX & TX
bool UART_Init(void)
{
	// default: async, 8 bit, 1 stop, no parity bit
	UCSR0A |= BV(U2X0);			// double speed mode
	UCSR0D |= BV(RXS)|BV(SFDE);	// clear & enable start frame detection on RXS to wake up on RX

	// read UBRR value from EEPROM & apply if valid
	while (!eeprom_is_ready());
	byte ubrr = eeprom_read_byte(BOOT_UBRR_EEP_ADDR);

	// otherwise load default value
	if (!UART_SetUbrr(ubrr, false))
	{
		UBRR0 = BOOT_UBRR_DEFAULT;
		return false;
	}
	
	return true;
}

// enable or disable UART without re-initializing
void UART_Enable(bool en)
{
	if (uartEnable == en) { return; } // nothing to do
	else { uartEnable = en; }

	if (uartEnable)
	{
		// flush buffers
		txBufIn = txBufOut = 0;
		rxBufIn = rxBufOut = 0;
		SET(UCSR0A, TXC0);	// clear TX complete flags
		CLR(UCSR0A, FE0);	// clear frame error flag
		UCSR0B |= BV(RXEN0)|BV(TXEN0)| BV(RXCIE0);	// enable RX, TX RX complete interrupt
	}
	else
	{
		// disable RX, TX & disable data register empty interrupt
		UCSR0B &= ~(BV(RXEN0)|BV(TXEN0)|BV(UDRIE0));
		uartBusy = false; // can't be busy if not enabled
	}
}

// send formatted string via UART
void UART_Printf(const char *formatstr, ...)
{
	// complete current transmission but don't accept new input
	if (!uartEnable) { return; }
	uartBusy = true;
	
	// variadic sorcery
	char buffer[UART_PRINT_BUF_SIZE] = {0};
	va_list args;
	va_start(args, formatstr);
	vsnprintf(buffer, UART_PRINT_BUF_SIZE, formatstr, args);
	va_end(args);

	// copy chars to TX buffer
	byte i=0;
	while (buffer[i]) { TxChar(buffer[i++]); }

	SET(UCSR0A, TXC0);		// clear transmit complete flag
	SET(UCSR0B, UDRIE0);	// enable data register empty interrupt
}

// read LF-terminated string from RX input buffer
bool UART_RxString(char* buffer)
{
	// nothing to do
	if (!uartEnable) { return false; }
	
	// handle out-of-sync condition
	if (rxFrameError)
	{
		// disabled RX & clear flag
		CLR(UCSR0B, RXEN0);
		rxFrameError = false;

		// invalidate UBRR value in EEPROM to force re-calibration on next boot
		// assuming wrong UBRR setting caused the frame error
		while (!eeprom_is_ready());
		eeprom_write_byte(BOOT_UBRR_EEP_ADDR, 0x00);
				
		// flush buffer & re-enable RX
		rxBufIn = rxBufOut = 0;
		SET(UCSR0B, RXEN0);

		return false;
	}
	
	byte src = rxBufOut;
	byte dst = 0;
	
	// go through all unread chars of the buffer
	while (src != rxBufIn)
	{
		buffer[dst++] = rxBuffer[src++];	// write chars to string
		src %= UART_RX_BUF_SIZE;			// ring buffer wrap around
		
		// remove LF symbol from RX buffer, zero-terminate string
		if (rxBuffer[src] == '\n')
		{
			buffer[dst] = 0;
			rxBuffer[src] = 0;
			rxBufOut = rxBufIn;
			return true;
		}
	}
	
	// no LF terminated string received
	return false;
}

// return true if UART is busy transmitting
bool UART_TxBusy(void)
{
	if (!uartBusy) { return false; } // no
	
	// trying to sleep with data register empty interrupt still enabled would immediately wake us up again
	if (GET(UCSR0B, UDRIE0)) { return true; }
	
	// going to sleep while TX is still active damages last char
	if (!GET(UCSR0A, TXC0)) { return true; }
	
	// TX complete
	uartBusy = false;
	return false;
}

// set UBRR register
bool UART_SetUbrr(byte ubrr, bool write_to_eep)
{
	// sanity check
	if (UART_VerifyUbrr(ubrr))
	{
		// wait for TX complete before changing UBRR
		while (UART_TxBusy());
		UBRR0 = ubrr;

		if (write_to_eep)
		{
			// write UBRR value to EEPROM for bootloader & first UART_Init()
			while (!eeprom_is_ready());
			eeprom_update_byte(BOOT_UBRR_EEP_ADDR, ubrr);
		}
		return true;
	}
	
	return false;
}

// sanity check UBRR value, return true if valid
bool UART_VerifyUbrr(byte ubrr)
{
	return ((ubrr >= BOOT_UBRR_MIN) && (ubrr <= BOOT_UBRR_MAX));
}

// calculate UBRR value based on RC clock frequency measurement
byte UART_CalcUbrr(uint32_t f_rc)
{
	return (byte)roundf((f_rc/(8.0f*UART_BAUDRATE)-1.0f));
}

// run UART calibration, optional write to EEPROM
bool UART_Calibrate(bool eep_write)
{
	UART_Printf("UART calibration.. ");
	
	uint32_t f_rc = RTC_GetRcOscFreq();			// measure RC oscillator frequency
	byte ubrr = UART_CalcUbrr(f_rc);			// calculate UBRR
	bool ok = UART_SetUbrr(ubrr, eep_write);	// sanity check & apply
	
	UART_Printf("%s!\nf_rc: %lu, UBRR: %u\n", (ok?"OK":"ERROR"), f_rc, ubrr);

	return ok;
}

// returns true if UART is enabled
bool UART_GetEnabled(void)
{
	return uartEnable;
}

// write single char to TX buffer
static void TxChar(byte in)
{
	txBuffer[txBufIn++] = in;
	txBufIn %= UART_TX_BUF_SIZE;	// wrap around
}

// TX data empty interrupt
ISR(USART0_UDRE_vect)
{
	SET(UCSR0A, TXC0);				// manually clear TX complete flag
	UDR0 = txBuffer[txBufOut++];	// write data byte to UART from TX buffer, this clears UDRE flag
	txBufOut %= UART_TX_BUF_SIZE;	// ring buffer wrap around
	
	// buffer empty -> disable data register empty interrupt, otherwise it will keep triggering
	if (txBufOut == txBufIn) { CLR(UCSR0B, UDRIE0); }
}

// RX Interrupt
ISR(USART0_RX_vect)
{		
	rxFrameError = GET(UCSR0A, FE0);	// get frame error flag
	rxBuffer[rxBufIn++] = UDR0;			// read data byte from UART into RX buffer, this clears RXC flag
	rxBufIn %= UART_RX_BUF_SIZE;		// ring buffer wrap around
}

// -------------------------------------- EOF --------------------------------------