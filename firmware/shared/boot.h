/*
===============================================================================
 Project	: O.S.I.R.I.S.
 Author		: Nicolai Sawilla (0xCAFEAFFE)
 Licence	: GNU GPL v2
 Version	: v2.0 (PCB Rev 2.0)
 Content	: Defines needed in both bootloader and application 
===============================================================================
*/

// UBRR value loaded on (de)fault and limits
#define BOOT_UBRR_DEFAULT		0x22	// 8.0MHz
#define BOOT_UBRR_MIN			0x20	// 7.6MHz
#define BOOT_UBRR_MAX			0x23	// 8.4MHz
#define BOOT_UBRR_EEP_ADDR		0x00

// define pin that is connected to USB
#define BOOT_VUSB_PORT          PIND
#define BOOT_VUSB_PIN			2u