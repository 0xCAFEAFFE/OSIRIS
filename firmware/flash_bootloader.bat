avrdude -c usbasp -p m328pb -e -U flash:w:".\bootloader\optiboot.hex":i
pause