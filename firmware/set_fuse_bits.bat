avrdude -c usbasp -p m328pb -P usb -U lfuse:w:0xE2:m -U hfuse:w:0xD6:m -U efuse:w:0xFF:m 
pause