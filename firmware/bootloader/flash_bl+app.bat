@echo off
avrdude -c usbasp -p m328pb -P usb -e -U lfuse:w:0xE2:m -U hfuse:w:0xD6:m -U efuse:w:0xFF:m -U flash:w:".\optiboot.hex":i
avrdude -c usbasp -p m328pb -P usb -D -U flash:w:"..\application\OSIRIS_FW.hex":i
pause