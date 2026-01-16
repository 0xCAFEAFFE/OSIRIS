
## Project OSIRIS

The **O**pen **S**ource **I**onizing **R**adiation **I**ndication **S**ystem can serve as a standalone Geiger counter or dosimeter and as a data logger or true random number generator when connected to a PC. It uses widely available components and is optimized for low power consumption, making it well suited for portable operation.

:construction: The documentation is still under construction. :construction:

### Hardware Overview

The system is powered by a single cell LiPo battery through a soft-latch and MCP1640 boost converter. A high voltage supply provides 400V to the infamous SBM-20 G-M tube for radiation detection, a pulse amplifier shapes the signal for processing by the ATmega328PB microcontroller at the core of the system. A 32kHz clock crystal provides an accurate time base to the MCU. The battery can be charged through the Micro-USB connector and MCP73831 linear charger, the CH340 USB-UART bridge enumerates the device as a virtual serial COM port when connected to a PC, which can be used for updating the firmware, configuration, debugging and logging. A low power, reflective LCD, a foil keypad and an electromechanical beeper form a lean user interface, a 3D-printed snap-fit enclosure keeps everything together in a portable and compact form factor.

The circuit schematics and layout overview is released under *CC BY-NC-SA 4.0* in the `hardware/electronics` folder of this repository.

To deter copycats from profiting off this project without contributing, the manufacturing files are currently not freely available but the bare PCB is available for a symbolic fee in my [Tindie store](https://www.tindie.com/stores/0xcafeaffe/), a partially populated kit is available there as well.

The STL files for the enclosure are released under *CC BY-NC-SA 4.0* and are available in the `hardware/mechanical` folder of this repository and on [Thingiverse](https://www.thingiverse.com/thing:7263135).

### Firmware Overview

The firmware is written in C and implements a interrupt-driven, modular approach using a super-loop. The MCU spends most time in sleep mode to save power but will be woken up by various interrupts, most importantly the G-M radiation events and the 1s tick interrupt provided by the clock XTAL, which triggers the calculation of filtered radiation measurement values. A simple command parser allows the user to control the device through a serial terminal, independently of the UI. To compensate the relatively poor accuracy of the internal RC oscillator, the UART baud rate setting can be calibrated against the external clock XTAL and stored to EEPROM, a modified version of the OptiBoot bootloader loads that calibrated UART setting, which allows for faster firmware upload, which is especially useful during development.

The firmware is released under *GPL v2.0* and available in the `firmware` folder of this repository. It includes a heavily modified version of a [LCD driver library](https://www.lcd-module.de/lcd-tft-beispiel-code-programmierung/application-note/arduino.html), released under *GPL v2.0* as well as a modified version of [OptiBoot](https://github.com/Optiboot/optiboot), released under *GPL v2.0* as well.

### User Interface

Each key has primary functions assigned for short and long key presses:

| Press | Red                   | Yellow          | Green                | 
|-------|-----------------------|-----------------|----------------------|
| Short | Power on / Mute alarm | Toggle clicker  | Cycle view mode      |
| Long  | Power off             | Toggle key lock | Cycle mode setting   |

The following view modes can be cycled through by shortly pressing the green key, in some of which the green key has an alternative function if pressed long:

| View Mode   | Green Long Function   |  
|-------------|-----------------------|
| Dose Rate   | Cycle Filter Setting  |
| Total Dose  | Reset Total Dose      |
| Time        | Reset System Time     |
| Alarm Level | Cycle Alarm Level     |
| Voltages    | -                     |

The LCD symbols (no always visible) in the top right corner, from left to right:

 - Keylock enabled
 - Clicker enabled
 - USB connected
 - Battery state
 
The UART calibration routine can be started by holding the yellow key while powering on the device.

### Command Parser

The command parser reads ASCII input from the serial port and expects commands to be terminated with a `LF` character (`'\n', 0x0A`).
Each command consists of a single letter and an optional argument, e.g.: `a` reads out the current alarm level, `a0.5` sets the level to 0.5µSv/h, sending `?` lists all available commands.

The default Windows driver settings for the CH340 cause it to reset the MCU whenever it is connected to the PC or when the COM port is opened. To avoid that, change the following settings:

    Device Manager -> USB-SERIAL CH340 -> Properties -> Port Settings -> Advanced 

```
- [ ] Enabling The Serial Port Enumerator
- [x] Disable Modem Handshake
```

### Build Tools

The application was developed using [Microchip Studio](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio), `v7.0.2594` which ships with the quite old version of `avr-gcc v5.4.0` and `DFP v2.3.518`, the project file for Microchip Studio can be found in the `mcp studio` folder.

If you prefer to build manually and/or with a more recent compiler version, then I suggest downloading this [release by ZakKemble](https://github.com/ZakKemble/avr-gcc-build/releases/tag/v15.1.0-1), which uses the same versions of `avr-gcc v15.1.0`, `binutils v2.44` and `avr-libc v2.2.1` as the latest toolchain [release by Microchip](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio/gcc-compilers), but also includes matching version of `make` and `avrdude` for Windows. The Makefile was only tested on Windows but *should* work for Linux as well. Yes, I'm a Windows person, shame on me, I know, I know..

The bootloader was compiled using `avr-gcc v15.1.0` as well, resulting in a 510 byte binary which *barely* fits the boot section size of 512 bytes. The binary was smaller with the older `avr-gcc v5.4.0`, future compiler versions *might* produce a binary too large to fit.

Fuse settings: Low =0xE2, High =0xD6, Extended =0xFF.

While flashing the bootloader via ISP, USB must be connected to provide power to the system.

In case the default UART setting is way off, the bootloader might not receive the firmware correctly. I'd therefore recommend, when flashing the bootloader via ISP for the first time, to once flash the application via ISP as well, to already have the UART calibration routine in place. Once the application runs, it will auto-calibrate the UART and from there on the bootloader should work as well.


### Warnings :warning:

 - The linear charger is set to a current of 300mA, the connected battery therefore needs to have a capacity of at least 300mAh for a safe charging rate of 1C. 
 - Don't let the battery charge unattended.
 - Don't touch the high voltage path of the PCB while power is on
 - The capacitors stay charged for some for a while after turning off the device.

### Limits

I would expect the device to work up to ~1mSv/h but I don't have a test source strong enough to verify that.

The so called TRNG is only a very primitive draft and the quality of the random numbers was not verified at all.

### Disclaimer 

This device should not be considered a scientific instrument or safety equipment but an experimental development kit for the qualified, radiation-curious professional.

### Contact

Questions and feedback are appreciated!

105112757+0xCAFEAFFE@users.noreply.github.com
