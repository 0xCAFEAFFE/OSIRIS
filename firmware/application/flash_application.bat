@echo off
echo Enter serial port number, e.g. COM7:
set /p comport=
avrdude -v -V -c arduino -p atmega328pb -P %comport% -b 28800 -D -U flash:w:"./OSIRIS_FW.hex":i 
pause