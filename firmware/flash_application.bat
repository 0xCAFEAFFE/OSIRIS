avrdude -v -V -c arduino -p atmega328pb -P COM7 -b 28800 -D -U flash:w:"./microchip studio/OSIRIS_FW/Debug/OSIRIS_FW.hex":i 
pause