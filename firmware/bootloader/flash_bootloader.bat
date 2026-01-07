@echo off
avrdude -c usbasp -p m328pb -P usb -e -U flash:w:".\OSIRIS_BL.hex":i
pause